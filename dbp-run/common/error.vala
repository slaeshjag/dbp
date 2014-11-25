using Gtk;

namespace ErrorLog {
	bool use_gui = false;
	
	/*public string[] errormessage = {
		"arne",
		"lol",
	};*/
	
	public void init(ref unowned string[] args) {
		use_gui = Gtk.init_check(ref args);
	}
	
	public void display(string message) {
		if(use_gui) {
			var msgbox = new MessageDialog(null, DialogFlags.MODAL, MessageType.ERROR, ButtonsType.OK, "Error: %s\n", message);
			msgbox.run();
		} else {
			stdout.printf("Error: %s\n", message);
		}
	}
}
