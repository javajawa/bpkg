bpkg-build
==========

Stripped down and optimised tool chain for generating simple debian packages.

It is intended for use where a closed group, such as a company, is building
large packages on a regular basis.

It intends to provide the following:

 - Running entirely as unpriviledged
 - Simple method for creating packages from existing sources.
 - I/O streamlining to help with large packages.
 - Configure user and group of files going into the package.

The configuration is not directly debian-compatible.
The build is controlled by a Makefile in the root directory,
which much contain two targets: `build`, and `manifest`.
`build` may be phony, but `manifest` should provide a `manifest`
file which lists all of the files in the current directory
which should form the pacakge (excluding those in debian/).

Optionally, an `owners` files can be supplied.
Each line is for tab separated values: `path`, `user`, `group`, and `mask`.
Files that begin with `path` (literal string comparison)
will be created with `user:group` ownership and the permissions
in mask, less all executable bits if the file on disk is not
executable.

Rules are processed in the order supplied in the file, so the
last defined rule that matches is applied.
It is up to the package's `preinst` script to ensure any users or
groups exist.
Folders that do not exist in the manifest will be created with
the default permissions on the target machine.

If no rules match, or no owner file exists, all files will be
`root:root 0755` (less executable bits).

An example owners file for a user's account files might look like:

```
/home/benedict	benedict	benedict	02711
/home/benedict/	benedict	benedict	00700
/home/benedict/.ssh	benedict	benedict	00701
```
