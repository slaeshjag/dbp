[CCode(cname="GETTEXT_PACKAGE")]
extern const string GETTEXT_PACKAGE;

struct Config {
	bool gui_errors;
}

void usage() {
	stdout.printf(_("Runs a DBP file's default application\n"));
	stdout.printf(_("Version %s\n"), DBP.Config.version());
	stdout.printf(_("Usage: dbp-run-path [--gui-errors] <path to .dbp> [arg1] [arg2] ...\n"));
}

int main(string[] args) {
	string[] argv = {};
	bool parse_option;
	int error_ret;
	Config config = Config() {
		gui_errors = false
	};
	ErrorLog errorlog;
	
	errorlog = new ErrorLog(ref args);
	
	Intl.setlocale(LocaleCategory.MESSAGES, "");
	Intl.textdomain(GETTEXT_PACKAGE); 
	Intl.bind_textdomain_codeset(GETTEXT_PACKAGE, "utf-8"); 
	DBP.Config.init();
	
	if ((error_ret = DBP.ServerAPI.connect()) < 0) {
		errorlog.display(DBP.Error.lookup(error_ret));
		return 1;
	}
	
	parse_option = true;
	foreach(string s in args[1:args.length]) {
		if(s.length < 1 || s[0] != '-' || s[1] != '-')
			parse_option = false;
		
		if(parse_option) {
			switch(s) {
				case "--gui-errors":
					config.gui_errors = true;
					break;
				default:
					stderr.printf(_("Error: unknown option '%s'\n"), s);
					usage();
					return 1;
				case "--help":
					usage();
					return 0;
			}
		} else {
			argv += s;
		}
	}
	
	if(argv.length == 0) {
		usage();
		return 1;
	}

	Run.package_path = argv[0];

	try {
		Run.run_path(argv[0], argv[1:argv.length]);
	} catch(Error e) {
		errorlog.display(e.message);
		return 1;
	}
	
	
	return 0;
}
