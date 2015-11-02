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
	enum ErrorEnum {
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
		NOTFOUND;
		
		public string error_string() {
			//TODO: fix
			return this.to_string();
		}
	}
	
	[CCode (cheader_filename = "config.h")]
	namespace Config {
		[CCode (cname = "struct DBPConfig", has_type_id = false)]
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
			
			[CCode (array_length_cname = "archs", array_length_type = "int")]
			string[] arch;

			string? run_script;

			Desktop df;
		}
		
		[CCode (cname = "dbp_config_struct")]
		Config config;
		
		[CCode (cprefix = "DBP_CONFIG_")]
		public const string FILE_PATH;
	
		[CCode (cname = "dbp_config_init")]
		void init();
		[CCode (cname = "dbp_config_version_get")]
		public unowned string version();
	}

	[CCode (cheader_filename = "desktop.h", cname = "struct DBPDesktopFile", free_function = "dbp_desktop_free")]
	[Compact]
	public class Desktop {
		[CCode (cname = "dbp_desktop_parse")]
		public Desktop(char[] str);
		[CCode (cname = "dbp_desktop_parse_file")]
		public Desktop.from_file(string path);
		[CCode (cname = "dbp_desktop_write")]
		public void write(string path);
		[CCode (cname = "dbp_desktop_lookup")]
		public unowned string? lookup(string key, string locale, string section);
		[CCode (cname = "dbp_desktop_lookup_section")]
		public int lookup_section(string section);
		[CCode (cname = "dbp_desktop_lookup_entry")]
		public int lookup_entry(string key, string locale, int section);
	}
	
	[CCode (cheader_filename = "loop.h", cprefix = "dbp_loop_")]
	namespace Loop {
		int mount(string image, string id, string user, string src_mount, string appdata);
		void umount(string pkg_id, int loop, string user);
		int directory_setup(string path, int umask);
	}
	
	[CCode (cheader_filename = "meta.h")]
	namespace Meta {
		[CCode (cname = "struct DBPMetaPackage", has_type_id = false, destroy_function = "", default_value = "{}")]
		public struct Package {
			[CCode (cname = "df")]
			Desktop desktop_file;
			string section;
		}
		
		[CCode (cname = "dbp_meta_package_open")]
		int package_open(string path, out Package mp);
	}

	[CCode (cheader_filename = "dbpmgr/dbpmgr.h", cprefix = "dbpmgr_error_")]
	namespace Error {
		[CCode (cname = "dbpmgr_error_lookup")]
		unowned string lookup(int error);
	}
	
	[CCode (cheader_filename = "dbpmgr/dbpmgr.h")]
	namespace Depend {
		[CCode (cname = "dbpmgr_depend_debian_check")]
		bool debian_check(string? package_string);

		
		[CCode (cname = "struct DBPDependList", has_type_id = false, destroy_function = "", default_value = "{}")]
		public struct ListEntry {
			[CCode (array_length_cname = "depends", array_length_type = "int")]
			string?[] depend;
		}

		[CCode (cname = "struct DBPDependListList", free_function = "dbpmgr_depend_delete_list_ptr")]
		[Compact]
		public class List {
			[CCode (cname = "dbpmgr_depend_check")]
			public List(DBP.Desktop df);
			public ListEntry sysonly;
			public ListEntry dbponly;
			public ListEntry syspref;
			public ListEntry dbppref;
			public ListEntry whatevs;
		}
	}

	[CCode (cheader_filename = "dbpmgr/dbpmgr.h", cprefix = "dbpmgr_server_")]
	namespace ServerAPI {
		[CCode (cname = "dbpmgr_server_connect")]
		int connect();
		[CCode (cname = "dbpmgr_server_ping")]
		int ping();
		[CCode (cname = "dbpmgr_server_mount")]
		int mount(string pkg_id, string user);
		[CCode (cname = "dbpmgr_server_umount")]
		int umount(int mount_ref);
		[CCode (cname = "dbpmgr_server_mountpoint_get")]
		int mountpoint_get(string pkg_id, out string mountpoint);
		[CCode (cname = "dbpmgr_server_register_path")]
		int register_path(string path, out string pkg_id);
		[CCode (cname = "dbpmgr_server_unregister_path")]
		int unregister_path(string path);
		[CCode (cname = "dbpmgr_server_id_from_path")]
		int id_from_path(string path, out string id);
		[CCode (cname = "dbpmgr_server_path_from_id")]
		int path_from_id(string id, out string path);
		
		[CCode (cheader_filename = "dbpmgr/dbpmgr.h", cname = "struct DBPList", free_function = "dbpmgr_server_package_list_free")]
		[Compact]
		public class PackageList {
			[CCode (cname = "dbpmgr_server_package_list")]
			public PackageList();
			public unowned PackageList next;
			public string id;
			public string path;
			public bool on_desktop;
		}



	}
}
