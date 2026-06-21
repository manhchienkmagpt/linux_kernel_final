#include "ui_network_page.h"
#include "network_info.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

typedef struct {
    AppContext *ctx;
    GtkWidget *iface_list;
    GtkWidget *name_label;
    GtkWidget *ipv4_label;
    GtkWidget *mac_label;
    GtkWidget *state_label;
    GtkWidget *sent_label;
    GtkWidget *recv_label;
} NetworkPage;

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

static void set_detail(NetworkPage *page, const char *iface) {
    char *address_path = g_strdup_printf("/sys/class/net/%s/address", iface);
    char *state_path = g_strdup_printf("/sys/class/net/%s/operstate", iface);
    char *tx_path = g_strdup_printf("/sys/class/net/%s/statistics/tx_bytes", iface);
    char *rx_path = g_strdup_printf("/sys/class/net/%s/statistics/rx_bytes", iface);

    char *ipv4 = ipv4_for_interface(iface);
    char *mac = read_trimmed_file(address_path);
    char *state = read_trimmed_file(state_path);
    char *sent = read_trimmed_file(tx_path);
    char *recv = read_trimmed_file(rx_path);

    gtk_label_set_text(GTK_LABEL(page->name_label), iface);
    gtk_label_set_text(GTK_LABEL(page->ipv4_label), ipv4);
    gtk_label_set_text(GTK_LABEL(page->mac_label), mac);
    gtk_label_set_text(GTK_LABEL(page->state_label), state);
    gtk_label_set_text(GTK_LABEL(page->sent_label), sent);
    gtk_label_set_text(GTK_LABEL(page->recv_label), recv);

    g_free(address_path);
    g_free(state_path);
    g_free(tx_path);
    g_free(rx_path);
    g_free(ipv4);
    g_free(mac);
    g_free(state);
    g_free(sent);
    g_free(recv);
}

static void on_iface_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    NetworkPage *page = user_data;
    const char *iface = row ? g_object_get_data(G_OBJECT(row), "iface") : NULL;
    if (iface) set_detail(page, iface);
}

static void clear_interfaces(NetworkPage *page) {
    GtkWidget *child = gtk_widget_get_first_child(page->iface_list);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(page->iface_list), child);
        child = next;
    }
}

static void add_interface(NetworkPage *page, const char *name) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(name);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_set_margin_top(label, 8);
    gtk_widget_set_margin_bottom(label, 8);
    gtk_widget_set_margin_start(label, 10);
    gtk_widget_set_margin_end(label, 10);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
    g_object_set_data_full(G_OBJECT(row), "iface", g_strdup(name), g_free);
    gtk_list_box_append(GTK_LIST_BOX(page->iface_list), row);
}

static void refresh_interfaces(NetworkPage *page) {
    clear_interfaces(page);
    GDir *dir = g_dir_open("/sys/class/net", 0, NULL);
    if (!dir) {
        log_page_append(page->ctx->log_page, "ERROR", "Cannot open /sys/class/net.");
        return;
    }
    const char *name;
    gboolean first = TRUE;
    while ((name = g_dir_read_name(dir)) != NULL) {
        add_interface(page, name);
        if (first) {
            set_detail(page, name);
            first = FALSE;
        }
    }
    g_dir_close(dir);

    char *info = get_network_info();
    log_page_append(page->ctx->log_page, "INFO", info);
    g_free(info);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_interfaces(user_data);
}

static GtkWidget *detail_row(const char *label, GtkWidget **value) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *left = gtk_label_new(label);
    gtk_widget_set_size_request(left, 140, -1);
    gtk_label_set_xalign(GTK_LABEL(left), 0.0f);
    *value = gtk_label_new("N/A");
    gtk_label_set_xalign(GTK_LABEL(*value), 0.0f);
    gtk_widget_set_hexpand(*value, TRUE);
    gtk_box_append(GTK_BOX(row), left);
    gtk_box_append(GTK_BOX(row), *value);
    return row;
}

GtkWidget *ui_network_page_new(AppContext *ctx) {
    NetworkPage *page = g_new0(NetworkPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Network");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);

    GtkWidget *content = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_box_append(GTK_BOX(root), content);

    page->iface_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(page->iface_list), GTK_SELECTION_SINGLE);
    GtkWidget *left_scroll = gtk_scrolled_window_new();
    gtk_widget_set_size_request(left_scroll, 240, -1);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(left_scroll), page->iface_list);
    gtk_paned_set_start_child(GTK_PANED(content), left_scroll);

    GtkWidget *detail = gtk_frame_new("Interface Detail");
    GtkWidget *detail_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(detail_box, 14);
    gtk_widget_set_margin_bottom(detail_box, 14);
    gtk_widget_set_margin_start(detail_box, 14);
    gtk_widget_set_margin_end(detail_box, 14);
    gtk_frame_set_child(GTK_FRAME(detail), detail_box);
    gtk_box_append(GTK_BOX(detail_box), detail_row("Interface Name", &page->name_label));
    gtk_box_append(GTK_BOX(detail_box), detail_row("IPv4", &page->ipv4_label));
    gtk_box_append(GTK_BOX(detail_box), detail_row("MAC", &page->mac_label));
    gtk_box_append(GTK_BOX(detail_box), detail_row("State", &page->state_label));
    gtk_box_append(GTK_BOX(detail_box), detail_row("Bytes Sent", &page->sent_label));
    gtk_box_append(GTK_BOX(detail_box), detail_row("Bytes Received", &page->recv_label));
    gtk_paned_set_end_child(GTK_PANED(content), detail);

    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    g_signal_connect(page->iface_list, "row-selected", G_CALLBACK(on_iface_selected), page);
    refresh_interfaces(page);
    return root;
}
