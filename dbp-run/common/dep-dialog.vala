//[CCode(cname="GETTEXT_PACKAGE")]
//extern const string GETTEXT_PACKAGE;


private class DepWarning : Gtk.Box {
	Gtk.Label warning_msg;
	Gtk.Image warning_icon;

	public DepWarning(string package_name) {
		StringBuilder sb = new StringBuilder();
		sb.append_printf(_("One or more dependencies for package '%s' is missing. The program may or may not work without these. Below is a list of missing dependencies."), package_name);
		warning_icon = new Gtk.Image.from_icon_name("gtk-dialog-warning", Gtk.IconSize.DIALOG);
		warning_msg = new Gtk.Label(sb.str);
		warning_msg.wrap_mode = Pango.WrapMode.WORD_CHAR;
		warning_msg.wrap = true;
		this.orientation = Gtk.Orientation.HORIZONTAL;
		this.set_spacing(8);
		this.pack_start(this.warning_icon, false, false, 16);
		this.pack_start(this.warning_msg, true, false, 0);
		this.show_all();
	}
}


private class DepListWidget : Gtk.Expander {
	Gtk.ScrolledWindow scrollw;
	Gtk.TreeView tree;

	public void append_list(string[]? list) {
		Gtk.TreeIter ti;
		Gtk.ListStore li;
		string[] s;

		if (list == null)
			return;
		li = (Gtk.ListStore) this.tree.get_model();
		foreach (string current in list) {
			s = current.split(";");
			li.append(out ti);
			li.set(ti, 0, s[0], 1, s[1]);
		}
	}

	public DepListWidget(string label, string[]? list) {
		this.set_label(label);
		scrollw = new Gtk.ScrolledWindow(null, null);
		tree = new Gtk.TreeView();
		tree.set_model(new Gtk.ListStore(2, typeof(string), typeof(string)));
		tree.insert_column_with_attributes(-1, _("Package name"), new Gtk.CellRendererText(), "text", 0);
		tree.insert_column_with_attributes(-1, _("Version"), new Gtk.CellRendererText(), "text", 1);

		if (list != null && list.length > 0)
			this.set_expanded(true);
		append_list(list);
		
		this.add(scrollw);
		scrollw.add(tree);
		this.show_all();
	}
}


public class DepListDialog {
	DepWarning depwarn;
	Gtk.Dialog dialog;
	DepListWidget sysex;
	DepListWidget dbpex;

	public enum Result {
		INSTALL,
		ABORT,
		LAUNCH,
	}

	public int run() {
		int ret;
		ret = dialog.run();
		dialog.destroy();
		dialog = null;
		while (Gtk.events_pending())
			Gtk.main_iteration();
		return ret;
	}

	private void handle_collapse(Gtk.Expander ex, Gtk.Box box, bool sig) {
		box.set_child_packing(ex, (sig && !ex.get_expanded()) || (!sig && ex.get_expanded()), true, 0, Gtk.PackType.START);
	}

	public DepListDialog(string package_name, string[]? sysdep, string[]? dbpdep) {
		Gtk.Button install, launch, abort;

		dialog = new Gtk.Dialog();
		depwarn = new DepWarning(package_name);

		dialog.get_content_area().pack_start(depwarn, false, false, 16);
		sysex = new DepListWidget(_("System packages"), sysdep);
		dialog.get_content_area().pack_start(sysex, true, true, 0);
		dbpex = new DepListWidget(_("Movable packages"), dbpdep);
		dialog.get_content_area().pack_start(dbpex, true, true, 0);

		install = new Gtk.Button.with_label(_("Install missing dependencies"));
		install.set_always_show_image(true);
		install.set_image(new Gtk.Image.from_icon_name("gtk-floppy", Gtk.IconSize.BUTTON));
		launch = new Gtk.Button.with_label(_("Launch anyway"));
		launch.set_always_show_image(true);
		launch.set_image(new Gtk.Image.from_icon_name("gtk-yes", Gtk.IconSize.BUTTON));
		abort = new Gtk.Button.with_label(_("Abort launch"));
		abort.set_always_show_image(true);
		abort.set_image(new Gtk.Image.from_icon_name("gtk-no", Gtk.IconSize.BUTTON));
		this.dialog.add_action_widget(install, Result.INSTALL);
		this.dialog.add_action_widget(abort, Result.ABORT);
		this.dialog.add_action_widget(launch, Result.LAUNCH);
		handle_collapse(this.sysex, this.dialog.get_content_area(), false);
		handle_collapse(this.dbpex, this.dialog.get_content_area(), false);
		this.sysex.activate.connect(() => {handle_collapse(this.sysex, this.dialog.get_content_area(), true);});
		this.dbpex.activate.connect(() => {handle_collapse(this.dbpex, this.dialog.get_content_area(), true);});

		this.dialog.set_default_size(500, 350);
		this.dialog.show_all();
	}
}
