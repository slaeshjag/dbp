namespace DepCheck {
	string[] check_dbp_dep(string[] dep) throws IOError {
		string[] missing = { };
		foreach (string s in dep) {
			/* check if the package is present */
			string path;
			int err = DBP.ServerAPI.path_from_id(s, out path);
			if (err < 0)
				missing += s;
		}

		return missing;
	}

	string[] check_sys_dep(string[] deps, string pkgarch) {
//		DBP.DebDepCheck.do_init();
		string[] missing = { };

		foreach (string s in deps) {
			if (!DBP.Depend.debian_check(s))
				missing += s;
		}

//		DBP.DebDepCheck.do_free();
		return missing;
	}
}
