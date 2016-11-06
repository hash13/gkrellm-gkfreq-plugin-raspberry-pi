/*
    GKrellM-gkfreq
    
    A plugin to GKrellM that displays the current CPU frequencies.
    
    Authors:
    Erik Kjellson <erikiiofph7@users.sourceforge.net>
    Brad Davis <brad@peakunix.net> (version 1.0)
    
    Copyright (C) 2005-2014

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Using code from gkx86info http://anchois.free.fr/
 * with patches from whatdoineed2do@yahoo.co.uk
 * and knefas@gmail.com
 */

#include <gkrellm2/gkrellm.h>
#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>


// Version of the plugin
#define PLUGIN_VERSION          "2.4"
// My email address
#define EMAIL_ADDRESS           "erikiiofph7@users.sourceforge.net"
// Sourceforge homepage for the plugin_info_text&firstvalue
#define HOMEPAGE_URL            "http://sourceforge.net/projects/gkrellm-gkfreq/"

// STYLE_NAME will be the theme subdirectory for custom images for this
//  plugin and it will be the gkrellmrc style name for custom settings.
#define	STYLE_NAME              "gkrellm-gkfreq"
// CONFIG_NAME would be the name in the configuration tree.
#define	CONFIG_NAME             "gkrellm-gkfreq"
// The MONITOR_CONFIG_KEYWORD is used to mark the lines in the config 
// file that belongs to this plugin.
#define MONITOR_CONFIG_KEYWORD  "gkrellm-gkfreq"
// Variable name for mode
#define CFG_VAR_NAME_MODE       "mode"
// Max length of variable names in config file.
#define CFG_VAR_NAME_MAXLEN     31

#define PLUGIN_PLACEMENT  MON_INSERT_AFTER|MON_CPU

// Maximum number of CPU cores to support
#define MAX_NUM_CPU             64
// Spacings between the text labels
#define SPACING_BETWEEN_ROWS    1
#define SPACING_BETWEEN_COLS    0

// Define modes - DON'T CHANGE THE NUMBERS, THEY HAVE TO BE ADDED IN NUMERIC ORDER LATER ON...
#define MODE_VAL_ALL            0
#define MODE_LBL_ALL            "Show all CPU frequencies"
#define MODE_VAL_MAXAVGMIN      1
#define MODE_LBL_MAXAVGMIN      "Show max, avg & min CPU frequency"
#define MODE_VAL_MAX            2
#define MODE_LBL_MAX            "Show maximum CPU frequency"
#define MODE_VAL_AVG            3
#define MODE_LBL_AVG            "Show average CPU frequency"
#define MODE_VAL_MIN            4
#define MODE_LBL_MIN            "Show minimum CPU frequency"

// Max length of text on the About tab in the config window.
#define ABOUT_STRING_MAXLEN     350


// Definition of CPU structure
struct GKFreqStruct{
    GkrellmDecal  *label_cpu;
    gint          freq;
    GkrellmDecal  *label_freq;
};

static GkrellmMonitor *monitor;
static GkrellmPanel   *panel;
static gint           style_id;

static struct    GKFreqStruct cpu[MAX_NUM_CPU];
static gint      num_cpu; // number of CPUs
       GtkWidget *vbox_panel;
       GtkWidget *comboMode;
       gint      mode;

// Get the CPU frequency (in MHz) for CPU number cpu_desired
static gint get_cpu_freq(gint cpu_desired) {
  // Variables needed for reading the file
  FILE *f;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  
  // Variables for finding freq for the desired CPU.
  int cpu_freq = -1;   // CPU frequency that was found.
  char fileName[100];
  sprintf( fileName, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu_desired );
  f = fopen(fileName, "r");

 // f = fopen("/proc/cpuinfo", "r");
  if (f == NULL && cpu_desired ==0) {
    printf("ERROR: GKRELLM-GKREQ: couldnt open cpufreq for reading (%s).\n",fileName);
    return -1;
  }
  
  // Step through the file content line by line.
  while ((read = getline(&line, &len, f)) != -1) {
	cpu_freq = atoi( line )/1000;
  }
  fclose(f);
  free(line);
  return cpu_freq;
}

// Callback function to be run when a panel is exposed for the first time.
static gint panel_expose_event(GtkWidget *widget, GdkEventExpose *ev) {

  gdk_draw_pixmap(widget->window,
    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
    panel->pixmap, ev->area.x, ev->area.y, ev->area.x, ev->area.y,
    ev->area.width, ev->area.height);
  return FALSE;
}

// Redrawing the graphic components of the plugin.
static void update_plugin() {
  gint i;
  
  // Get all CPU frequencies and calculate max, avg & min
  for (i=0; i<num_cpu; i++) {
    cpu[i].freq = get_cpu_freq(i);
  }
  gint freq_max = cpu[0].freq;
  for (i=1; i<num_cpu; i++) {
    freq_max = fmax(freq_max, cpu[i].freq);
  }
  gint freq_avg = 0;
  for (i=0; i<num_cpu; i++) {
    freq_avg += cpu[i].freq;
  }
  freq_avg = freq_avg/num_cpu;
  gint freq_min = cpu[0].freq;
  for (i=1; i<num_cpu; i++) {
    freq_min = fmin(freq_min, cpu[i].freq);
  }

  gchar text_freq[20];
  // Check mode and create labels accordingly
  if (mode == MODE_VAL_MAXAVGMIN) {
    // Max
    gkrellm_draw_decal_text(panel, cpu[0].label_cpu, "Max", 0);    
    sprintf(text_freq, "%d MHz", freq_max);
    gkrellm_draw_decal_text(panel, cpu[0].label_freq, text_freq, 0);
    // Avg
    gkrellm_draw_decal_text(panel, cpu[1].label_cpu, "Avg", 0);    
    sprintf(text_freq, "%d MHz", freq_avg);
    gkrellm_draw_decal_text(panel, cpu[1].label_freq, text_freq, 0);
    // Min
    gkrellm_draw_decal_text(panel, cpu[2].label_cpu, "Min", 0);    
    sprintf(text_freq, "%d MHz", freq_min);
    gkrellm_draw_decal_text(panel, cpu[2].label_freq, text_freq, 0);
    
  }else if (mode == MODE_VAL_MAX) {
    gkrellm_draw_decal_text(panel, cpu[0].label_cpu, "Max", 0);    
    sprintf(text_freq, "%d MHz", freq_max);
    gkrellm_draw_decal_text(panel, cpu[0].label_freq, text_freq, 0);
    
  }else if (mode == MODE_VAL_AVG) {
    gkrellm_draw_decal_text(panel, cpu[0].label_cpu, "Avg", 0);    
    sprintf(text_freq, "%d MHz", freq_avg);
    gkrellm_draw_decal_text(panel, cpu[0].label_freq, text_freq, 0);
    
  }else if (mode == MODE_VAL_MIN) {
    gkrellm_draw_decal_text(panel, cpu[0].label_cpu, "Min", 0);    
    sprintf(text_freq, "%d MHz", freq_min);
    gkrellm_draw_decal_text(panel, cpu[0].label_freq, text_freq, 0);  
    
  }else {
    // Mode: Show all CPUs
    // Step through all CPUs and redraw their components.
    for (i=0; i<num_cpu; i++) {
      gchar text_cpu[10];
      sprintf(text_cpu, "CPU%d", i);
      gkrellm_draw_decal_text(panel, cpu[i].label_cpu, text_cpu, 0);
      sprintf(text_freq, "%d MHz", cpu[i].freq);
      gkrellm_draw_decal_text(panel, cpu[i].label_freq, text_freq, 0);
    }
  }
  gkrellm_draw_panel_layers(panel);

}


// Initialize the graphic components of the plugin.
static void create_plugin(GtkWidget *vbox, gint first_create) {

  gint i;
  
  GkrellmStyle      *style;
  GkrellmTextstyle  *ts, *ts_alt;
  
   // Initialize panel if this is the first time this routine is run.
  if (first_create) {
    panel = gkrellm_panel_new0();
  }

  // Save the Gtk vbox to be able to call this function in the future.
  vbox_panel = vbox;

  // Get style
  style = gkrellm_meter_style(style_id);

  // Get font styles, ts_alt is usually smaller than ts.
  ts = gkrellm_meter_textstyle(style_id);
  ts_alt = gkrellm_meter_alt_textstyle(style_id);

  // Count the number of CPUs
  if (first_create) {
    for (num_cpu=0; num_cpu<MAX_NUM_CPU; num_cpu++){
      if (get_cpu_freq(num_cpu) < 0) {
        break;
      }
    }
  }
  
  // Calculate x pos for frequency label                          
  gint x;
  if ((mode == MODE_VAL_ALL) && (num_cpu > 10)) {
    x = gdk_string_width(gdk_font_from_description(ts_alt->font), "CPUXX") + SPACING_BETWEEN_COLS;
  } else {
    x = gdk_string_width(gdk_font_from_description(ts_alt->font), "CPUX") + SPACING_BETWEEN_COLS;
  }
  
  // Check mode and create labels accordingly
  gint freqs2show;
  if ((mode == MODE_VAL_MAX) || (mode == MODE_VAL_AVG) || (mode == MODE_VAL_MIN)) {
    freqs2show = 1;
  } else if (mode == MODE_VAL_MAXAVGMIN) {
    freqs2show = 3;    
  } else {
    // Mode: MODE_VAL_ALL
    freqs2show = num_cpu;
  }
  // Step through all frequencies to show
  gint y = -1; /* y = -1 places at top margin  */
  for (i=0; i<freqs2show; i++) {
    // Create a decal for the cpu label
    cpu[i].label_cpu = gkrellm_create_decal_text(panel,
                            "CPUX", /* string used for vertical sizing */
                            ts_alt,
                            style,
                            -1,     /* x = -1 places at left margin */
                            y,     
                            -1);    /* use full width */
    // Create a decal for the frequency label
    cpu[i].label_freq = gkrellm_create_decal_text(panel,
                            "XXXX MHz", /* string used for vertical sizing */
                            ts,
                            style,
                            x, 
                            y,     
                            -1);    /* use full width */
    // Calculate y pos for next row
    if (i == 0) {
      y = cpu[i].label_cpu->y;
    }
    y = y + fmax(cpu[i].label_cpu->h, cpu[i].label_freq->h) + SPACING_BETWEEN_ROWS; 
  }
  
  // Configure the panel to hold the above created decals, and create it.
  gkrellm_panel_configure(panel, NULL, style);
  gkrellm_panel_create(vbox, monitor, panel);

  // Connect callback function for expose event.
  if (first_create)
      g_signal_connect(G_OBJECT (panel->drawing_area), "expose_event",
              G_CALLBACK (panel_expose_event), NULL);
}


/* Save any configuration data we have in config lines in the format:
     MONITOR_CONFIG_KEYWORD  config_keyword  data
 */
static void save_plugin_config(FILE *f) {
  // Save mode
  fprintf(f, "%s %s %d\n", MONITOR_CONFIG_KEYWORD, CFG_VAR_NAME_MODE, mode);
}


/* When GKrellM is started up, load_plugin_config() is called if any
   config lines for this plugin are found.  The lines must have been
   saved by save_plugin_config().
 */
static void load_plugin_config(gchar *config_line) {
  gchar   config_keyword[CFG_VAR_NAME_MAXLEN+1], config_value[CFG_BUFSIZE];
  gint    n;
    
  // Get the keyword and the corresponding data for this config line.
  if ((n = sscanf(config_line, "%s %[^\n]", 
                                 config_keyword, config_value)) != 2) {
    return;
  }
  // Check keyword
  if (strcmp(config_keyword, CFG_VAR_NAME_MODE) == 0) {
    // Load the number of visible counters
    sscanf(config_value, "%d", &mode);
  }
}


/* The apply is called whenever the user hits the OK or the Apply
   button in the config window.
 */
static void apply_plugin_config(void) {
    // Get the chosen mode
    gchar *chosen =  gtk_combo_box_get_active_text(GTK_COMBO_BOX(comboMode));
    if (strcmp(chosen, MODE_LBL_MAXAVGMIN) == 0) {
      mode = MODE_VAL_MAXAVGMIN;
    } else if (strcmp(chosen, MODE_LBL_MAX) == 0) {
      mode = MODE_VAL_MAX;
    } else if  (strcmp(chosen, MODE_LBL_AVG) == 0) {
      mode = MODE_VAL_AVG;
    } else if  (strcmp(chosen, MODE_LBL_MIN) == 0) {
      mode = MODE_VAL_MIN;
    } else {
      mode = MODE_VAL_ALL;
    } 

    // Re-create all panel contents (needed if switching between ALL and something else).
    gkrellm_panel_destroy(panel);
    create_plugin(vbox_panel, 1);
}

// Creates a tab for the plugin config window.
static void create_plugin_tab(GtkWidget *tab_vbox) {
    GtkWidget       *tabs;
    GtkWidget       *text;
    GtkWidget       *vbox;
    GtkWidget       *table;

    // Create Gtk user config widgets.
    tabs = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);

    // Options tab
    vbox  = gkrellm_gtk_framed_notebook_page(tabs, "Options");
    table = gtk_table_new(1,     /* Number of rows */
                          2,     /* Number of columns*/
                          FALSE); /* TRUE = homogenous, all cells of equal size */
    gtk_box_pack_start(GTK_BOX(vbox), table,
                            FALSE, /* TRUE = expand */
                            FALSE, /* TRUE = fill */
                            0);    /* padding */
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table), 2);
    // Row 1: Mode selection
    gtk_table_attach_defaults(GTK_TABLE(table), gtk_label_new("Mode:"),
                            1,  /* Column number to attach left side to */
                            2,  /* Column number to attach right side to */
                            0,  /* Row number to attach top to */
                            1); /* Row number to attach bottom to */
    // Combo box options have to be added in numeric order!
    comboMode = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(comboMode), MODE_LBL_ALL);
    gtk_combo_box_append_text(GTK_COMBO_BOX(comboMode), MODE_LBL_MAXAVGMIN);
    gtk_combo_box_append_text(GTK_COMBO_BOX(comboMode), MODE_LBL_MAX);
    gtk_combo_box_append_text(GTK_COMBO_BOX(comboMode), MODE_LBL_AVG);
    gtk_combo_box_append_text(GTK_COMBO_BOX(comboMode), MODE_LBL_MIN);
    gtk_combo_box_set_active(GTK_COMBO_BOX(comboMode), mode);
    gtk_table_attach_defaults(GTK_TABLE(table), comboMode,
                            2,    /* Column number to attach left side to */
                            3,    /* Column number to attach right side to */
                            0,  /* Row number to attach top to */
                            1); /* Row number to attach bottom to */
    
    // About tab
    // Text that will show up in the "About" tab of the config window.
    static gchar about_text[ABOUT_STRING_MAXLEN+1];
    sprintf(about_text, "%s version %s\n"
                        "A plugin to GKrellM that displays the current CPU frequencies.\n\n"
                        "%s\n\n"
                        "Authors:\n"
                        "  Erik Kjellson <%s>\n\n"
                        "  Brad Davis <brad@peakunix.net> (version 1.0)\n"
                        "Copyright (C) 2005-2014\n"
                        "Released under the GNU General Public License\n"
                        ,CONFIG_NAME, PLUGIN_VERSION, HOMEPAGE_URL, EMAIL_ADDRESS);
    vbox = gkrellm_gtk_framed_notebook_page(tabs, "About");
    text = gkrellm_gtk_scrolled_text_view(vbox, NULL,
                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gkrellm_gtk_text_view_append(text, &about_text[0]);
}

// Mandatory GKrellM plugin data structure
static GkrellmMonitor	plugin_mon = {

  CONFIG_NAME,        /* Name, for config tab.    */
  0,                  /* Id,  0 if a plugin       */
  create_plugin,      /* The create function      */
  update_plugin,      /* The update function      */
  create_plugin_tab,  /* The config tab create function   */
  apply_plugin_config,/* Apply the config function        */

  save_plugin_config,  /* The save_plugin_config() function  */
  load_plugin_config,  /* The load_plugin_config() function  */
  MONITOR_CONFIG_KEYWORD, /* config keyword                     */

  NULL,               /* Undefined 2  */
  NULL,               /* Undefined 1  */
  NULL,               /* private      */

  PLUGIN_PLACEMENT,   /* Insert plugin before this monitor.       */

  NULL, /* Handle if a plugin, filled in by GKrellM     */
  NULL  /* path if a plugin, filled in by GKrellM       */
};


// Mandatory initialization routine for the plugin.
GkrellmMonitor * gkrellm_init_plugin() {

  mode = MODE_VAL_ALL;
  
  style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
  monitor = &plugin_mon;
  return &plugin_mon;
}
