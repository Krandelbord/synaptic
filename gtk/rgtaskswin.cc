/* rgtaskswin.cc
 *
 * Copyright (c) 2004 Michael Vogt
 *
 * Author: Michael Vogt <mvo@debian.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <gtk/gtk.h>
#include <cassert>
#include <list>
#include "config.h"
#include "rgtaskswin.h"
#include "rgmainwindow.h"
#include "i18n.h"

enum {
   TASK_CHECKBOX_COLUMN,
   TASK_NAME_COLUMN,
   TASK_DESCR_COLUMN,
   TASK_N_COLUMNS
};

void RGTasksWin::cbButtonOkClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonOkClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;
   
   GtkTreeIter iter;
   GtkTreeModel *model = GTK_TREE_MODEL(me->_store);
   if(!gtk_tree_model_get_iter_first(model, &iter)) {
      me->hide();
      return;
   }

   gboolean res = FALSE;
   gchar *taskname = NULL;
   string cmd = "/usr/bin/tasksel";
   do {
      gtk_tree_model_get(model, &iter, 
			 TASK_CHECKBOX_COLUMN, &res, 
			 TASK_NAME_COLUMN, &taskname,
			 -1);
      if(res) {
	 //cout << "selected: " << taskname << endl;
	 cmd += " --task-packages " + string(taskname);
      }
   } while(gtk_tree_model_iter_next(model, &iter));

   char buf[255];
   vector<string> packages;
   FILE *f = popen(cmd.c_str(), "r");
   while(fgets(buf, 254, f) != NULL) {
      packages.push_back(string(g_strstrip(buf)));
   }
   pclose(f);

#if 0
   cout << "got: " << endl;
   for(int i=0;i<packages.size();i++) {
      cout << packages[i] << endl;
   }
#endif

   me->hide();

   me->_mainWin->selectToInstall(packages);

}

void RGTasksWin::cbButtonCancelClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonCancelClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;
   me->hide();
}

void RGTasksWin::cbButtonDetailsClicked(GtkWidget *self, void *data)
{
   //cout << "cbButtonDetailsClicked(GtkWidget *self, void *data)"<<endl;
   RGTasksWin *me = (RGTasksWin*)data;
   
}


void RGTasksWin::cell_toggled_callback (GtkCellRendererToggle *cell,
					gchar *path_string,
					gpointer user_data)
{
   GtkTreeIter iter;
   gboolean res;

   GtkTreeModel *model = (GtkTreeModel*)user_data;
   GtkTreePath* path = gtk_tree_path_new_from_string(path_string);
   gtk_tree_model_get_iter(model, &iter, path);
   gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
		      TASK_CHECKBOX_COLUMN, &res, -1);
   gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		      TASK_CHECKBOX_COLUMN, !res,
		      -1);
}


RGTasksWin::RGTasksWin(RGWindow *parent) 
   : RGGladeWindow(parent, "tasks")
{
   _mainWin = (RGMainWindow *)parent;

   GtkListStore *store= _store = gtk_list_store_new (TASK_N_COLUMNS, 
						     G_TYPE_BOOLEAN, 
						     G_TYPE_STRING, 
						     G_TYPE_STRING);
   
   // fiel in tasks
   char buf[255];
   gchar **strArray;
   FILE *f = popen("/usr/bin/tasksel --list-tasks","r");
   while(fgets(buf,254,f) != NULL) {
      strArray = g_strsplit(buf, "\t", 2);
      g_strstrip(strArray[1]);

      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  TASK_CHECKBOX_COLUMN, FALSE,
			  TASK_NAME_COLUMN, strArray[0],
			  TASK_DESCR_COLUMN, strArray[1],
			  -1);
      g_strfreev(strArray);
   }
   pclose(f);
   GtkWidget *tree;
   
   tree = glade_xml_get_widget(_gladeXML, "treeview_tasks");
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   renderer = gtk_cell_renderer_toggle_new ();
   g_object_set(renderer, "activatable", TRUE, NULL);
   g_signal_connect(renderer, "toggled", 
		    (GCallback) cell_toggled_callback, store);
   column = gtk_tree_view_column_new_with_attributes ("Install",
                                                      renderer,
                                                      "active", TASK_CHECKBOX_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Taskname",
						      renderer,
						      "text", TASK_NAME_COLUMN,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   column = gtk_tree_view_column_new_with_attributes ("Description",
						      renderer,
						      "text", TASK_DESCR_COLUMN,
						      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
   

   gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
   
   glade_xml_signal_connect_data(_gladeXML, "on_button_ok_clicked",
			    (GCallback) cbButtonOkClicked, this);
   glade_xml_signal_connect_data(_gladeXML, "on_button_cancel_clicked",
                    (GCallback) cbButtonCancelClicked, this);
   glade_xml_signal_connect_data(_gladeXML, "on_button_details_clicked",
                    (GCallback) cbButtonDetailsClicked, this);

};

