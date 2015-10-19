int main(string[] args) {
	DBP.ServerAPI.PackageList pl;
	unowned DBP.ServerAPI.PackageList next;
	if (DBP.ServerAPI.connect() < 0)
		return -1;
	
	pl = new DBP.ServerAPI.PackageList();
	
	for (next = pl; next != null; next = next.next) {
		stdout.printf("'%s' '%s' '%s'\n", next.id, next.path, next.on_desktop?"true":"false");
	}
	return 0;
}
