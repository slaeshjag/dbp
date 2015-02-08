namespace DepCheck {
	string[] check_dbp_dep(string[] dep) throws IOError {
		string[] missing = { };
		foreach (string s in dep) {
			/* check if the package is present */
			string path = bus.path_from_id(s);
			if (path == "" || path == "!" || path == null)
				missing += s;
		}

		return missing;
	}

	string[] check_sys_dep(string[] deps) {
		DBP.DebDepCheck.do_init();
		string[] missing = { };

		foreach (string s in deps) {
			/* TODO: send package arch instead of "any" */
			if (!DBP.DebDepCheck.check_package(s, "any"))
				missing += s;
		}

		DBP.DebDepCheck.do_free();
		return missing;
	}
}
