using Gtk;

public class ErrorLog {
	bool use_gui = false;
	
	public string[] errormessage = new string[] {
		_("The DBP System Daemon is not responding. Make sure it's running"),
		_("An unknown internal error has occured. This is only supposed to be a placeholder. You should examine the DBP System daemon log-file for further information."),
		_("The system has ran out of loopback device nodes. Try closing some applications and try again."),
		_("The system has ran out of loopback device nodes. Try closing some applications and try again."),
		_("The DBP System Daemon was unable to assign the package file to a device node. ioctl failed."),
		_("The package file could not be opened. Make sure it's still accessable."),
		_("Unable to allocate system resources. You're probably out of RAM. Close some programs and try again."),
		_("You've attempted to launch a program from a package that could not be found in the local database. This is likely a bug."),
		_("Unable to access the package filesystem. This DBP-file is likely corrupt."),
		_("The meta-data in this package is malformed: Illegal character(s) was found in a generated mountpoint."),
		_("Unable to set up AuFS union. This is likely a bug. This may temporarily be worked around by rebooting."),
		_("Your user doesn't have the permissions needed to create the application data directory on the SD-card that this application is installed on."),
		_("No default.desktop was found in the meta-data for this package."),
		_("The package with this ID is already registered! This is not an error, and you shouldn't see this message."),
		_("An unhandled error has occured. This is just a placehoder that you shouldn't see."),
		_("Unable to read meta-data from package. The package file is likely corrupt. Try re-downloading it."),
		_("This package lacks a default executable, and therefore cannot be executed directly."),
		_("The program was killed by a signal"),
		_("The program has crashed from a segmentation fault. This likely a bug in the program."),
		_("The program exited abnormally for an unknown reason.")
	};
	
	public ErrorLog(ref unowned string[] args) {
		use_gui = Gtk.init_check(ref args);
	}
	
	public void display_errno(int number) {
		number = number*(-1) - 1000;
		if(number < 0 || number >= errormessage.length)
			return;
		display(errormessage[number]);
	}
	
	public void display(string message) {
		string error_msg, path, package_id;
		if (Run.package_path == null)
			path = _("Unknown");
		else
			path = Run.package_path;

		if (Run.package_id == null)
			package_id = _("Unknown");
		else
			package_id = Run.package_id;
		
		error_msg = _("Error: ") + message + "\n\n" + _("Path: ") + path + "\n" + _("Package ID: ") + package_id;
		if(use_gui) {
			if (int.parse(message) < 0)
				display_errno(int.parse(message));
			else {
				MessageDialog msgbox = new MessageDialog(null, DialogFlags.MODAL, MessageType.ERROR, ButtonsType.OK, error_msg, message);
				msgbox.run();
			}
		} else {
			stdout.printf(error_msg);
		}
	}
}
