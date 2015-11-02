namespace Run {
	const int appdata_mode = 0755;

	string? package_path = null;
	string? package_id = null;
	
	class OutputLogger {
		int fd;
		string filename;
		unowned FileStream stream;
		Thread<int> thread;
		
		public OutputLogger(int fd, string filename, FileStream stream) {
			this.filename = filename;
			this.fd = fd;
			this.stream = stream;
			thread = new Thread<int>("output-logger", run);
		}
		
		private int run() {
			FileStream f = FileStream.open(filename, "wb");
			ssize_t bytes;
			uint8 buf[256];
			
			while(true) {
				bytes = Posix.read(fd, buf, buf.length);
				if(bytes <= 0)
					break;
				f.write(buf[0:bytes], 1);
				stream.write(buf[0:bytes], 1);
				stream.flush();
			}
			Posix.close(fd);
			return 0;
		}
	}

	bool dependency_ok(string pkg_id, DBP.Meta.Package pkg, bool gui_errors) throws IOError {
		string[] sysmissing = {}, dbpmissing = {};
		string pkgarch;
		DBP.Depend.List deplist = new DBP.Depend.List(pkg.desktop_file);
		
		pkgarch = pkg.desktop_file.lookup("Arch", "", "Package Entry");
		if (pkgarch == null || pkgarch == "")
			pkgarch = "any";
		if (pkgarch != "any") {
			bool supported = false;
			foreach (string s in DBP.Config.config.arch)
				if (s == pkgarch)
					supported = true;
			if (!supported)
				throw new IOError.FAILED(_("This package is not supported on your system architecture") + "\n" + _("Package is for") + " " + pkgarch);
		}

		foreach (string s in deplist.sysonly.depend)
			sysmissing += s;
		foreach (string s in deplist.syspref.depend)
			sysmissing += s;
		foreach (string s in deplist.dbponly.depend)
			dbpmissing += s;
		foreach (string s in deplist.dbppref.depend)
			dbpmissing += s;
		foreach (string s in deplist.whatevs.depend)
			dbpmissing += s;

		if (dbpmissing.length > 0 || sysmissing.length > 0) {
			string err;
			
			if (!gui_errors) {
				err = _("Package '%s' is missing some of its dependencies\n\n").printf(pkg_id);
				if (dbpmissing.length > 0) {
					err = err + _("Removable packages: ");
					err = err + string.joinv(", ", dbpmissing) + "\n\n";
				}

				if (sysmissing.length > 0) {
					err = err + _("System packages: ");
					err = err + string.joinv(", ", sysmissing) + "\n\n";
				}

				err = err + _("The application may or may not work at all without these. It is recommended that you use a package manager to install the missing dependencies.");

				stdout.printf("%s\n", err);
			} else {
				int sel;
				/* TODO: Care about the users' selection */
				DepListDialog dld;
				dld = new DepListDialog(pkg_id, sysmissing, dbpmissing);
				sel = dld.run();
				if (sel == DepListDialog.Result.LAUNCH) {
					// Carry on... //
				} else if (sel == DepListDialog.Result.INSTALL) {
					throw new IOError.FAILED(_("Automatic installation of dependencies is currently not supported\n"));
				} else
					return false;
				dld = null;
			}
		}
			
		return true;	
	}

	string resolve_appdata(string pkg_id, DBP.Meta.Package pkg) {
		string appdata_name; 
		
		appdata_name = pkg.desktop_file.lookup("Appdata", "", "Package Entry");
		
		if (appdata_name == null)
			appdata_name = pkg_id;

		return appdata_name;
	}

	void appdata_create(string pkg_id, string appdata_name) throws IOError {
		string mountpoint;
		int error_ret;
		string appdata, roappdata;
		
		
		if ((error_ret = DBP.ServerAPI.mountpoint_get(pkg_id, out mountpoint)) < 0)
			throw new IOError.FAILED(DBP.Error.lookup(error_ret));
		
		if(DBP.Config.config.per_user_appdata) {
			appdata = DBP.Config.config.data_directory + "_" + Environment.get_user_name();
			roappdata = DBP.Config.config.rodata_directory + "_" + Environment.get_user_name();
		} else {
			appdata = DBP.Config.config.data_directory;
			roappdata = DBP.Config.config.rodata_directory;
		}
	
		if (DBP.Config.config.per_package_appdata) {
			var dir = Path.build_filename(mountpoint, appdata, appdata_name);
			DirUtils.create_with_parents(dir, appdata_mode);
			if (!FileUtils.test(dir, FileTest.IS_DIR))
				throw new IOError.FAILED(_("Unable to create appdata directory") + " " + dir + "\n" + _("Make sure you have the file system permissions to create this directory"));
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata, appdata_name), appdata_mode);
		} else {
			var dir = Path.build_filename(mountpoint, appdata);
			DirUtils.create_with_parents(dir, appdata_mode);
			if (!FileUtils.test(dir, FileTest.IS_DIR))
				throw new IOError.FAILED(_("Unable to create appdata directory") + " " + dir + "\n" + _("Make sure you have the file system permissions to create this directory"));
			if (DBP.Config.config.create_rodata)
				DirUtils.create_with_parents(Path.build_filename(mountpoint, roappdata), appdata_mode);
		}
	}
	
	public void run(string pkg_id, string exec, string[] args, bool log, bool chdir, bool gui_errors) throws IOError, SpawnError, Error {
		int mount_id;
		string binary_path;
		string ?cwd;
		string[] argv = {};
		string appdata_name, pkgpath, run_script, pkg_arch;
		int outpipe, errpipe;
		string outlogfile, errlogfile;
		int pid;
		int info;
		int error_ret;
		DBP.Meta.Package pkg;
		OutputLogger stdoutlogger, stderrlogger;
		
		if (pkg_id == null)
			return;
	
		if ((error_ret = DBP.ServerAPI.path_from_id(pkg_id, out pkgpath)) < 0) {
			throw new IOError.FAILED(DBP.Error.lookup(error_ret));
		}
	
		Run.package_path = pkgpath;
		DBP.Meta.package_open(pkgpath, out pkg);
		
		if (!dependency_ok(pkg_id, pkg, gui_errors))
			return;

		appdata_name = resolve_appdata(pkg_id, pkg);
		appdata_create(pkg_id, appdata_name);
		
		//TODO: validate pkg_id
		pkg_arch = pkg.desktop_file.lookup("Arch", "", "Package Entry");
		if (pkg_arch == "any" || pkg_arch == null)
			pkg_arch = "";
		run_script = DBP.Config.config.df.lookup("run_script", pkg_arch, "Package Daemon Config");
		if (run_script == "")
			run_script = null;

		if ((mount_id = DBP.ServerAPI.mount(pkg_id, "")) < 0)
			throw new IOError.FAILED(DBP.Error.lookup(mount_id));
	
		binary_path = Path.build_filename(DBP.Config.config.union_mount, pkg_id, exec);
		cwd = chdir ? Path.build_filename(DBP.Config.config.union_mount, pkg_id) : null;
		
		if (run_script != null)
			argv += DBP.Config.config.run_script;

		argv += binary_path;
		foreach(string s in args)
			argv += s;
		
		if(log) {
			Process.spawn_async_with_pipes(cwd, argv, null, SpawnFlags.CHILD_INHERITS_STDIN | SpawnFlags.DO_NOT_REAP_CHILD, null, out pid, null, out outpipe, out errpipe);
			
			outlogfile = "%s-%s-stdout%s".printf(Path.build_filename(DBP.Config.config.dbpout_directory, DBP.Config.config.dbpout_prefix), pkg_id, DBP.Config.config.dbpout_suffix);
			errlogfile = "%s-%s-stderr%s".printf(Path.build_filename(DBP.Config.config.dbpout_directory, DBP.Config.config.dbpout_prefix), pkg_id, DBP.Config.config.dbpout_suffix);
			stdoutlogger = new OutputLogger(outpipe, outlogfile, stdout);
			stderrlogger = new OutputLogger(errpipe, errlogfile, stderr);
			
			Posix.waitpid(pid, out info, 0);
			Process.close_pid(pid);
		} else {
			Process.spawn_sync(cwd, argv, null, SpawnFlags.CHILD_INHERITS_STDIN, null, null, null);
		}
		
		DBP.ServerAPI.umount(mount_id);
	}
	
	public void run_path(string path_in, string[] args) throws IOError, SpawnError, Error {
		string pkg_id;
		string actual_path;
		string exec_name;
		string path;
		int error_ret;
		DBP.Meta.Package meta;
		ExecLine exec;
	
		if (path_in[0] != '/')
			path = Path.build_filename(Environment.get_current_dir(), path_in, null);
		else
			path = path_in;

		if ((error_ret = DBP.ServerAPI.register_path(path, out pkg_id)) < 0 && error_ret != DBP.ErrorEnum.PKG_REG)
			throw new IOError.FAILED(DBP.Error.lookup(error_ret));
		
		Run.package_id = pkg_id;

		// TODO: Should really check for errors here //
		DBP.ServerAPI.path_from_id(pkg_id, out actual_path);
		Run.package_path = actual_path;
		DBP.Meta.package_open(actual_path, out meta);
		exec_name = meta.desktop_file.lookup("Exec", "", "Desktop Entry");
		if (exec_name == null)
			throw new IOError.FAILED(_("Unable to extract exec from package meta data"));
		exec = new ExecLine(exec_name);
		exec.append(args);
		exec.run(false);
		
		if(error_ret != DBP.ErrorEnum.PKG_REG)
			DBP.ServerAPI.unregister_path(path);
	}
}
