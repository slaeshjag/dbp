dbp is an attempt at superseding the PND-system with something else that is
nicer to use.


Note to self:

Package Entry keys:

These are only looked at when they are in default.desktop
Entries that can hold multiple values are ;-separated

* Id			Package id
* Exec			(Optional) List of executables to export With enviroment override
* NoEnvExec		(Optional) List of executables to export
* Appdata		(Optional) appdata directory name
* SysDependency		(Optional) System packages needed to run
* PkgDependency		(Optional) DBP dependencies needed to run

### Potential repository-only keys
These are ignored by the DBP-system but might be of use in the repo
Potentially, a Package Entry might be present in all .desktop files with these
keys in, for per-application use.

* Maintainer		Doesn't really make sense for per-application, but the DBP-system doesn't care about it anyway
* Version		Probably useful for versioning packages. Maybe that the DBP-system should keep track of this value? Should probably decide on a version format too. Either that, or I should make a libdbp that exposes all package information.

