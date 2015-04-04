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

### DBus
Dbus-signals that may be of use:
* NewMeta	<path to .desktop>	- For every new .desktop file to be placed on the desktop
* RemoveMeta <path to .desktop>	- For every .desktop file that is to be removed from the desktop
* NewPackage <pkgid>		- For every new detected package
* RemovePackage <pkgid>		- For every removed package


### Potential repository-only keys
These are ignored by the DBP-system but might be of use in the repo
Potentially, a Package Entry might be present in all .desktop files with these
keys in, for per-application use.

* Maintainer		Doesn't really make sense for per-application, but the DBP-system doesn't care about it anyway
* Version		Probably useful for versioning packages. Maybe that the DBP-system should keep track of this value? Should probably decide on a version format too. Either that, or I should make a libdbp that exposes all package information.


### Code style for contributors
Code style is not that strict, although I prefer that it's kept consistent.
Indentation to be done with tabs (8 wide.)
Opening curly brackers should be placed on the same line as the scope declaration. Spaces between
function names/keywords and parathesis is optional. Spaces inside parenthesis is discouraged.
Function names are snake case, defines and enum values are all caps snake case. Types (structs, enum types)
are CamelCase. Variable names and labels are snake case.

The recommended background music while hacking is doom/gothic metal, eg. Katatonia. That's
the kind of general mood I'm in whenever I work on this anyway...
