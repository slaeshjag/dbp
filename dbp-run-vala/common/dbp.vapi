[CCode (cheader_filename = "dbp.h")]
namespace DBP {
	[CCode (cprefix = "DBP_")]
	public const string META_PREFIX;
	[CCode (cprefix = "DBP_")]
	public const string FS_NAME;
	[CCode (cprefix = "DBP_")]
	public const string UNIONFS_NAME;
	
	[CCode (cprefix = "DBP_DBUS_")]
	namespace DBus {
		public const string CLIENT_PREFIX;
		public const string DAEMON_PREFIX;
		public const string DAEMON_OBJECT;
	}
	
	[CCode (cname = "int", cprefix = "DBP_ERROR_", has_type_id = false)]
	enum Error {
		NO_REPLY,
		INTERNAL_MSG,
		NO_LOOP,
		SET_LOOP,
		SET_LOOP2,
		NO_PKG_ACCESS,
		NO_MEMORY,
		BAD_PKG_ID,
		BAD_FSIMG,
		ILL_DIRNAME,
		UNION_FAILED,
		APPD_NOPERM,
		NO_DEFAULTD,
		PKG_REG,
		UNHANDLED,
		BAD_META,
		NO_DEFAULTH,
		SIGEXIT,
		SIGSEGV,
		MYSTKILL,
	}
	
	[CCode (cheader_filename = "config.h")]
	namespace Config {
		[CCode (cname = "struct config_s", has_type_id = false)]
		struct Config {
			[CCode (array_length_cname = "file_extensions", array_length_type = "int")]
			string[] file_extension;
			//int file_extensions;
			[CCode (array_length_cname = "search_dirs", array_length_type = "int")]
			string[] search_dir;
			//int search_dirs;
			string img_mount;
			string union_mount;

			string data_directory;
			string rodata_directory;
			string icon_directory;
			string exec_directory;
			string desktop_directory;

			string dbpout_directory;
			string dbpout_prefix;
			string dbpout_suffix;

			string daemon_log;

			string exec_template;

			bool per_user_appdata;
			bool per_package_appdata;
			bool create_rodata;
		}
		
		[CCode (cname = "config_struct")]
		Config config;
		
		[CCode (cprefix = "CONFIG_")]
		public const string FILE_PATH;
	
		[CCode (cname = "config_init")]
		void init();
	}
	
	[CCode (cheader_filename = "desktop.h", cname = "struct desktop_file_s", free_function = "desktop_free")]
	[Compact]
	public class Desktop {
		[CCode (cname = "desktop_parse")]
		public Desktop(char[] str);
		[CCode (cname = "desktop_parse_file")]
		public Desktop.from_file(string path);
		[CCode (cname = "desktop_write")]
		public void write(string path);
		[CCode (cname = "desktop_lookup")]
		public unowned string? lookup(string key, string locale, string section);
		[CCode (cname = "desktop_lookup_section")]
		public int lookup_section(string section);
		[CCode (cname = "desktop_lookup_entry")]
		public int lookup_entry(string key, string locale, int section);
	}
	
	[CCode (cheader_filename = "loop.h", cprefix = "loop_")]
	namespace Loop {
		int mount(string image, string id, string user, string src_mount, string appdata);
		void umount(string pkg_id, int loop, string user);
		int directory_setup(string path, int umask);
	}
	
	[CCode (cheader_filename = "meta.h")]
	namespace Meta {
		[CCode (cname = "struct meta_package_s", has_type_id = false, destroy_function = "", default_value = "{}")]
		public struct Package {
			[CCode (cname = "df")]
			Desktop desktop_file;
			string section;
		}
		
		[CCode (cname = "meta_package_open")]
		int package_open(string path, out Package mp);
	}
}
