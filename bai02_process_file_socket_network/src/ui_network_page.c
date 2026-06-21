#include "ui_network_page.h"
#include "network_manager.h"
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

typedef struct {
    AppContext *ctx;

    GtkWidget *info_iface_combo;
    GtkWidget *info_ipv4_label;
    GtkWidget *info_mac_label;
    GtkWidget *info_state_label;
    GtkWidget *info_download_label;
    GtkWidget *info_upload_label;
    GtkWidget *info_rx_total_label;
    GtkWidget *info_tx_total_label;
    guint info_timer_id;
    gboolean info_has_prev;
    TrafficStats info_prev;
    gint64 info_prev_time;

    GtkWidget *conn_filter_combo;
    GtkWidget *conn_search_entry;
    GtkWidget *conn_list;

    GtkWidget *traffic_iface_combo;
    GtkWidget *download_label;
    GtkWidget *upload_label;
    GtkWidget *rx_total_label;
    GtkWidget *tx_total_label;
    guint traffic_timer_id;
    gboolean traffic_has_prev;
    TrafficStats traffic_prev;
    gint64 traffic_prev_time;
} NetworkPage;

typedef struct {
    NetworkPage *page;
    char *protocol;
    char *search;
} ConnectionRefreshTask;

typedef struct {
    NetworkPage *page;
    GPtrArray *items;
    char *error;
} ConnectionRefreshResult;

static GtkWidget *metric_row(const char *label, GtkWidget **value);

static GtkWidget *cell_label(const char *text, int width) {
    GtkWidget *label = gtk_label_new(text ? text : "");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(label, width, -1);
    return label;
}

static GtkWidget *table_header(const char **labels, const int *widths, int count) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(row, "heading");
    for (int i = 0; i < count; i++) {
        gtk_box_append(GTK_BOX(row), cell_label(labels[i], widths[i]));
    }
    return row;
}

static void clear_list_box(GtkWidget *list_box) {
    GtkWidget *child = gtk_widget_get_first_child(list_box);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(list_box), child);
        child = next;
    }
}

static void combo_reload_interfaces(GtkWidget *combo) {
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(combo));
    GPtrArray *interfaces = network_manager_list_interfaces();
    for (guint i = 0; i < interfaces->len; i++) {
        const char *iface = g_ptr_array_index(interfaces, i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), iface);
    }
    if (interfaces->len > 0) gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    g_ptr_array_free(interfaces, TRUE);
}

static void populate_interfaces(NetworkPage *page) {
    combo_reload_interfaces(page->info_iface_combo);
    combo_reload_interfaces(page->traffic_iface_combo);
}

static char *read_trimmed_file(const char *path) {
    char *content = NULL;
    if (!g_file_get_contents(path, &content, NULL, NULL)) return g_strdup("N/A");
    g_strstrip(content);
    return content;
}

static char *ipv4_for_interface(const char *iface_name) {
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) return g_strdup("N/A");

    char *result = g_strdup("N/A");
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || g_strcmp0(ifa->ifa_name, iface_name) != 0) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char host[NI_MAXHOST];
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0) {
                g_free(result);
                result = g_strdup(host);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
    return result;
}

static void update_info_static_labels(NetworkPage *page, const char *iface) {
    if (!iface || !*iface) return;

    char *mac_path = g_strdup_printf("/sys/class/net/%s/address", iface);
    char *state_path = g_strdup_printf("/sys/class/net/%s/operstate", iface);
    char *ipv4 = ipv4_for_interface(iface);
    char *mac = read_trimmed_file(mac_path);
    char *state = read_trimmed_file(state_path);

    gtk_label_set_text(GTK_LABEL(page->info_ipv4_label), ipv4);
    gtk_label_set_text(GTK_LABEL(page->info_mac_label), mac);
    gtk_label_set_text(GTK_LABEL(page->info_state_label), state);

    g_free(mac_path);
    g_free(state_path);
    g_free(ipv4);
    g_free(mac);
    g_free(state);
}

static void set_info_speed_labels(NetworkPage *page, double rx_speed, double tx_speed, TrafficStats *stats) {
    char *download = network_manager_format_speed(rx_speed);
    char *upload = network_manager_format_speed(tx_speed);
    char *rx_total = network_manager_format_bytes(stats->rx_bytes);
    char *tx_total = network_manager_format_bytes(stats->tx_bytes);
    gtk_label_set_text(GTK_LABEL(page->info_download_label), download);
    gtk_label_set_text(GTK_LABEL(page->info_upload_label), upload);
    gtk_label_set_text(GTK_LABEL(page->info_rx_total_label), rx_total);
    gtk_label_set_text(GTK_LABEL(page->info_tx_total_label), tx_total);
    g_free(download);
    g_free(upload);
    g_free(rx_total);
    g_free(tx_total);
}

static gboolean network_info_tick(gpointer user_data) {
    NetworkPage *page = user_data;
    char *iface = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page->info_iface_combo));
    if (!iface) return G_SOURCE_CONTINUE;

    update_info_static_labels(page, iface);

    TrafficStats now = {0};
    char *error = NULL;
    if (!network_manager_read_traffic(iface, &now, &error)) {
        log_page_append(page->ctx->log_page, "ERROR", error);
        g_free(error);
        g_free(iface);
        return G_SOURCE_CONTINUE;
    }

    gint64 current_time = g_get_monotonic_time();
    double rx_speed = 0.0;
    double tx_speed = 0.0;
    if (page->info_has_prev) {
        double delta = (current_time - page->info_prev_time) / 1000000.0;
        if (delta > 0.0) {
            rx_speed = (now.rx_bytes - page->info_prev.rx_bytes) / delta;
            tx_speed = (now.tx_bytes - page->info_prev.tx_bytes) / delta;
        }
    }

    page->info_prev = now;
    page->info_prev_time = current_time;
    page->info_has_prev = TRUE;
    set_info_speed_labels(page, rx_speed, tx_speed, &now);
    g_free(iface);
    return G_SOURCE_CONTINUE;
}

static void stop_network_info_monitor(NetworkPage *page) {
    if (page->info_timer_id) {
        g_source_remove(page->info_timer_id);
        page->info_timer_id = 0;
    }
    page->info_has_prev = FALSE;
}

static void on_info_start(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    if (page->info_timer_id) return;
    network_info_tick(page);
    page->info_timer_id = g_timeout_add_seconds(1, network_info_tick, page);
    log_page_append(page->ctx->log_page, "OK", "Network realtime info started.");
}

static void on_info_stop(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    stop_network_info_monitor(page);
    log_page_append(page->ctx->log_page, "INFO", "Network realtime info stopped.");
}

static void on_info_refresh_interfaces(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    populate_interfaces(page);
    network_info_tick(page);
    log_page_append(page->ctx->log_page, "INFO", "Network realtime interfaces refreshed.");
}

static void on_info_iface_changed(GtkComboBox *combo, gpointer user_data) {
    (void)combo;
    NetworkPage *page = user_data;
    page->info_has_prev = FALSE;
    network_info_tick(page);
}

static GtkWidget *network_info_tab(NetworkPage *page) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    page->info_iface_combo = gtk_combo_box_text_new();
    GtkWidget *start = gtk_button_new_with_label("Start Realtime");
    GtkWidget *stop = gtk_button_new_with_label("Stop Realtime");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh Interfaces");
    gtk_box_append(GTK_BOX(toolbar), gtk_label_new("Interface:"));
    gtk_box_append(GTK_BOX(toolbar), page->info_iface_combo);
    gtk_box_append(GTK_BOX(toolbar), start);
    gtk_box_append(GTK_BOX(toolbar), stop);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(root), toolbar);

    GtkWidget *metrics = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(metrics), metric_row("IPv4", &page->info_ipv4_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("MAC", &page->info_mac_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("State", &page->info_state_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("Download Speed", &page->info_download_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("Upload Speed", &page->info_upload_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("RX Total", &page->info_rx_total_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("TX Total", &page->info_tx_total_label));
    gtk_box_append(GTK_BOX(root), metrics);

    g_signal_connect(start, "clicked", G_CALLBACK(on_info_start), page);
    g_signal_connect(stop, "clicked", G_CALLBACK(on_info_stop), page);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_info_refresh_interfaces), page);
    g_signal_connect(page->info_iface_combo, "changed", G_CALLBACK(on_info_iface_changed), page);
    return root;
}

static void add_connection_row(NetworkPage *page, ConnectionInfo *info) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(box, 4);
    gtk_widget_set_margin_bottom(box, 4);
    gtk_widget_set_margin_start(box, 6);
    gtk_widget_set_margin_end(box, 6);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), box);
    gtk_box_append(GTK_BOX(box), cell_label(info->protocol, 80));
    gtk_box_append(GTK_BOX(box), cell_label(info->local_address, 220));
    gtk_box_append(GTK_BOX(box), cell_label(info->remote_address, 220));
    gtk_box_append(GTK_BOX(box), cell_label(info->state, 90));
    gtk_box_append(GTK_BOX(box), cell_label(info->program, 260));
    gtk_list_box_append(GTK_LIST_BOX(page->conn_list), row);
}

static void show_connections(NetworkPage *page, GPtrArray *items, const char *error) {
    clear_list_box(page->conn_list);
    if (error) {
        log_page_append(page->ctx->log_page, "ERROR", error);
    }
    for (guint i = 0; i < items->len; i++) {
        add_connection_row(page, g_ptr_array_index(items, i));
    }
    char *msg = g_strdup_printf("Connection Viewer refreshed. %u rows.", items->len);
    log_page_append(page->ctx->log_page, "INFO", msg);
    g_free(msg);
}

static gboolean connection_refresh_idle(gpointer user_data) {
    ConnectionRefreshResult *result = user_data;
    show_connections(result->page, result->items, result->error);
    g_ptr_array_free(result->items, TRUE);
    g_free(result->error);
    g_free(result);
    return FALSE;
}

static gpointer connection_refresh_worker(gpointer user_data) {
    ConnectionRefreshTask *task = user_data;
    ConnectionRefreshResult *result = g_new0(ConnectionRefreshResult, 1);
    result->page = task->page;
    result->items = network_manager_get_connections(task->protocol, task->search, &result->error);
    g_idle_add(connection_refresh_idle, result);
    g_free(task->protocol);
    g_free(task->search);
    g_free(task);
    return NULL;
}

static void refresh_connections(NetworkPage *page) {
    ConnectionRefreshTask *task = g_new0(ConnectionRefreshTask, 1);
    task->page = page;
    task->protocol = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page->conn_filter_combo));
    task->search = g_strdup(gtk_editable_get_text(GTK_EDITABLE(page->conn_search_entry)));
    GThread *thread = g_thread_new("connection-refresh", connection_refresh_worker, task);
    g_thread_unref(thread);
    log_page_append(page->ctx->log_page, "INFO", "Connection Viewer refresh requested.");
}

static void on_conn_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_connections(user_data);
}

static void on_conn_search_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    refresh_connections(user_data);
}

static void on_conn_filter_changed(GtkComboBox *combo, gpointer user_data) {
    (void)combo;
    refresh_connections(user_data);
}

static GtkWidget *connection_viewer_tab(NetworkPage *page) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(root), toolbar);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    page->conn_search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(page->conn_search_entry, TRUE);
    page->conn_filter_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(page->conn_filter_combo), "ALL");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(page->conn_filter_combo), "TCP");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(page->conn_filter_combo), "UDP");
    gtk_combo_box_set_active(GTK_COMBO_BOX(page->conn_filter_combo), 0);

    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(toolbar), gtk_label_new("Search:"));
    gtk_box_append(GTK_BOX(toolbar), page->conn_search_entry);
    gtk_box_append(GTK_BOX(toolbar), gtk_label_new("Protocol:"));
    gtk_box_append(GTK_BOX(toolbar), page->conn_filter_combo);

    const char *headers[] = {"Protocol", "Local Address", "Remote Address", "State", "PID/Program"};
    const int widths[] = {80, 220, 220, 90, 260};
    gtk_box_append(GTK_BOX(root), table_header(headers, widths, 5));

    page->conn_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->conn_list), GTK_SELECTION_NONE);
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page->conn_list);
    gtk_box_append(GTK_BOX(root), scroll);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_conn_refresh), page);
    g_signal_connect(page->conn_search_entry, "search-changed", G_CALLBACK(on_conn_search_changed), page);
    g_signal_connect(page->conn_filter_combo, "changed", G_CALLBACK(on_conn_filter_changed), page);
    return root;
}

static void set_traffic_labels(NetworkPage *page, double rx_speed, double tx_speed, TrafficStats *stats) {
    char *download = network_manager_format_speed(rx_speed);
    char *upload = network_manager_format_speed(tx_speed);
    char *rx_total = network_manager_format_bytes(stats->rx_bytes);
    char *tx_total = network_manager_format_bytes(stats->tx_bytes);
    gtk_label_set_text(GTK_LABEL(page->download_label), download);
    gtk_label_set_text(GTK_LABEL(page->upload_label), upload);
    gtk_label_set_text(GTK_LABEL(page->rx_total_label), rx_total);
    gtk_label_set_text(GTK_LABEL(page->tx_total_label), tx_total);
    g_free(download);
    g_free(upload);
    g_free(rx_total);
    g_free(tx_total);
}

static gboolean traffic_tick(gpointer user_data) {
    NetworkPage *page = user_data;
    char *iface = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(page->traffic_iface_combo));
    TrafficStats now = {0};
    char *error = NULL;
    if (!network_manager_read_traffic(iface, &now, &error)) {
        log_page_append(page->ctx->log_page, "ERROR", error);
        g_free(error);
        g_free(iface);
        return G_SOURCE_CONTINUE;
    }
    g_free(iface);

    gint64 current_time = g_get_monotonic_time();
    double rx_speed = 0.0;
    double tx_speed = 0.0;
    if (page->traffic_has_prev) {
        double delta = (current_time - page->traffic_prev_time) / 1000000.0;
        if (delta > 0.0) {
            rx_speed = (now.rx_bytes - page->traffic_prev.rx_bytes) / delta;
            tx_speed = (now.tx_bytes - page->traffic_prev.tx_bytes) / delta;
        }
    }
    page->traffic_prev = now;
    page->traffic_prev_time = current_time;
    page->traffic_has_prev = TRUE;
    set_traffic_labels(page, rx_speed, tx_speed, &now);
    return G_SOURCE_CONTINUE;
}

static void stop_traffic_monitor(NetworkPage *page) {
    if (page->traffic_timer_id) {
        g_source_remove(page->traffic_timer_id);
        page->traffic_timer_id = 0;
    }
    page->traffic_has_prev = FALSE;
}

static void on_traffic_start(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    if (page->traffic_timer_id) return;
    traffic_tick(page);
    page->traffic_timer_id = g_timeout_add_seconds(1, traffic_tick, page);
    log_page_append(page->ctx->log_page, "OK", "Traffic monitor started.");
}

static void on_traffic_stop(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    stop_traffic_monitor(page);
    log_page_append(page->ctx->log_page, "INFO", "Traffic monitor stopped.");
}

static void on_refresh_interfaces(GtkButton *button, gpointer user_data) {
    (void)button;
    NetworkPage *page = user_data;
    populate_interfaces(page);
    log_page_append(page->ctx->log_page, "INFO", "Network interfaces refreshed.");
}

static GtkWidget *metric_row(const char *label, GtkWidget **value) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *left = gtk_label_new(label);
    gtk_widget_set_size_request(left, 160, -1);
    gtk_label_set_xalign(GTK_LABEL(left), 0.0f);
    *value = gtk_label_new("0 B");
    gtk_label_set_xalign(GTK_LABEL(*value), 0.0f);
    gtk_widget_set_hexpand(*value, TRUE);
    gtk_box_append(GTK_BOX(row), left);
    gtk_box_append(GTK_BOX(row), *value);
    return row;
}

static GtkWidget *traffic_monitor_tab(NetworkPage *page) {
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    page->traffic_iface_combo = gtk_combo_box_text_new();
    GtkWidget *start = gtk_button_new_with_label("Start Monitor");
    GtkWidget *stop = gtk_button_new_with_label("Stop Monitor");
    GtkWidget *refresh = gtk_button_new_with_label("Refresh Interfaces");
    gtk_box_append(GTK_BOX(toolbar), gtk_label_new("Interface:"));
    gtk_box_append(GTK_BOX(toolbar), page->traffic_iface_combo);
    gtk_box_append(GTK_BOX(toolbar), start);
    gtk_box_append(GTK_BOX(toolbar), stop);
    gtk_box_append(GTK_BOX(toolbar), refresh);
    gtk_box_append(GTK_BOX(root), toolbar);

    GtkWidget *metrics = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_append(GTK_BOX(metrics), metric_row("Download Speed", &page->download_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("Upload Speed", &page->upload_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("RX Total", &page->rx_total_label));
    gtk_box_append(GTK_BOX(metrics), metric_row("TX Total", &page->tx_total_label));
    gtk_box_append(GTK_BOX(root), metrics);

    g_signal_connect(start, "clicked", G_CALLBACK(on_traffic_start), page);
    g_signal_connect(stop, "clicked", G_CALLBACK(on_traffic_stop), page);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh_interfaces), page);
    return root;
}

static void network_page_free(NetworkPage *page) {
    if (!page) return;
    stop_network_info_monitor(page);
    stop_traffic_monitor(page);
    g_free(page);
}

GtkWidget *ui_network_page_new(AppContext *ctx) {
    NetworkPage *page = g_new0(NetworkPage, 1);
    page->ctx = ctx;

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)network_page_free);

    GtkWidget *title = gtk_label_new("Network");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_box_append(GTK_BOX(root), notebook);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), network_info_tab(page), gtk_label_new("Network Info"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), connection_viewer_tab(page), gtk_label_new("Connection Viewer"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), traffic_monitor_tab(page), gtk_label_new("Traffic Monitor"));

    populate_interfaces(page);
    network_info_tick(page);
    refresh_connections(page);
    log_page_append(ctx->log_page, "INFO", "Network page loaded.");
    return root;
}
