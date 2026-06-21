#include "ui_dashboard_page.h"
#include "network_info.h"
#include "process_utils.h"
#include <stdio.h>
#include <sys/sysinfo.h>

typedef struct {
    AppContext *ctx;
    GtkWidget *cpu_value;
    GtkWidget *ram_value;
    GtkWidget *process_value;
    GtkWidget *network_value;
    GtkWidget *ip_value;
} DashboardPage;

static GtkWidget *card_new(const char *title, GtkWidget **value_label) {
    GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_frame_set_child(GTK_FRAME(frame), box);

    GtkWidget *title_label = gtk_label_new(title);
    gtk_widget_add_css_class(title_label, "dim-label");
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), title_label);

    *value_label = gtk_label_new("...");
    gtk_widget_add_css_class(*value_label, "title-2");
    gtk_widget_set_halign(*value_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), *value_label);
    return frame;
}

static double read_cpu_usage(void) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    int n = fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    if (n < 8) return 0.0;
    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long busy = total - idle - iowait;
    return total ? (busy * 100.0 / total) : 0.0;
}

static int count_processes(void) {
    char *list = get_process_list();
    int count = 0;
    if (list) {
        char **lines = g_strsplit(list, "\n", -1);
        for (int i = 1; lines[i]; i++) if (g_strstrip(lines[i])[0]) count++;
        g_strfreev(lines);
    }
    g_free(list);
    return count;
}

static char *first_ip_from_network_info(void) {
    char *info = get_network_info();
    char *ip = g_strdup("N/A");
    if (info) {
        char **lines = g_strsplit(info, "\n", -1);
        for (int i = 1; lines[i]; i++) {
            char **cols = g_strsplit(lines[i], "\t", -1);
            if (cols[0] && cols[1] && cols[2] && g_strcmp0(cols[1], "IPv4") == 0 && g_strcmp0(cols[2], "127.0.0.1") != 0) {
                g_free(ip);
                ip = g_strdup(cols[2]);
                g_strfreev(cols);
                break;
            }
            g_strfreev(cols);
        }
        g_strfreev(lines);
    }
    g_free(info);
    return ip;
}

static void refresh_dashboard(DashboardPage *page) {
    struct sysinfo si;
    sysinfo(&si);
    double ram_used = 0.0;
    if (si.totalram > 0) {
        ram_used = (double)(si.totalram - si.freeram) * 100.0 / (double)si.totalram;
    }

    char *cpu = g_strdup_printf("%.1f%%", read_cpu_usage());
    char *ram = g_strdup_printf("%.1f%%", ram_used);
    char *proc = g_strdup_printf("%d", count_processes());
    char *ip = first_ip_from_network_info();

    gtk_label_set_text(GTK_LABEL(page->cpu_value), cpu);
    gtk_label_set_text(GTK_LABEL(page->ram_value), ram);
    gtk_label_set_text(GTK_LABEL(page->process_value), proc);
    gtk_label_set_text(GTK_LABEL(page->network_value), g_strcmp0(ip, "N/A") == 0 ? "Offline/Unknown" : "Online");
    gtk_label_set_text(GTK_LABEL(page->ip_value), ip);
    log_page_append(page->ctx->log_page, "INFO", "Dashboard refreshed.");

    g_free(cpu);
    g_free(ram);
    g_free(proc);
    g_free(ip);
}

static void on_refresh(GtkButton *button, gpointer user_data) {
    (void)button;
    refresh_dashboard(user_data);
}

GtkWidget *ui_dashboard_page_new(AppContext *ctx) {
    DashboardPage *page = g_new0(DashboardPage, 1);
    page->ctx = ctx;
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    g_object_set_data_full(G_OBJECT(root), "page-data", page, (GDestroyNotify)g_free);

    GtkWidget *title = gtk_label_new("Dashboard");
    gtk_widget_add_css_class(title, "title-2");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), title);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_attach(GTK_GRID(grid), card_new("CPU Usage", &page->cpu_value), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_new("RAM Usage", &page->ram_value), 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_new("Total Processes", &page->process_value), 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_new("Network Status", &page->network_value), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_new("IP Address", &page->ip_value), 1, 1, 2, 1);
    gtk_box_append(GTK_BOX(root), grid);

    GtkWidget *refresh = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(root), refresh);
    g_signal_connect(refresh, "clicked", G_CALLBACK(on_refresh), page);
    refresh_dashboard(page);
    return root;
}
