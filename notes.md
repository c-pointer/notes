# notes 1 2023-09-10 "1.6" "User Commands"

## NAME
notes - manages note files.

## SYNOPSIS
COMMAND: notes [-s section] -a[!][e] name [file ...][-] -a[!]+[e] name [file ...][-] \
	-e[a] {name|pattern} -v[a] {name|pattern} -p[a] {name|pattern} -l [pattern] \
	-f pattern -d[a] {name|pattern} -r oldname newname -c rcfile

## DESCRIPTION
The notes-files are stored in a user-defined directory with optional subdirectories.
The subdirectories in the application are named _sections_.
Normally this directory is `~/.notes` or `~/Nextcloud/Notes` if there is **Nextcloud**.
It can be specified in the configuration file.
The files can be plain text, markdown or anything else that can configured
by the _rule_ command in the configuration file.
If the _note_ is `-` then it reads from **stdin**.

Running program without arguments, enters in TUI mode (**ncurses** interface).

The program was designed to behave as the `man` command:
```
# show page(s) of section (i.e. subdirectory) 'unix'
# whose title begins with 'sig11'
$ notes -s unix sig11

# search and show notes for a title with patterns
$ notes '*sig*'
```

### Naming
Notes naming used in forms, a) just title, b) section/title, c) title.type, d) section/title.type.
We can edit, view, move, rename, etc by using any of these forms.

## OPTIONS

#### -a[!], --add[!]
Creates a new note file. If file extension is not specified then the default will be used (see [notesrc 5](man)).
If additional files are specified in the command line, their contents will be inserted into the new note.
Use it with `-e` to invoke the editor or `-` to get input from **stdin**.
If the name is already used in this section, then an error will be issued;
use `!` option to replace the existing file,
or set the clobber variable to _false_ in the configuration file.

```
# example 1: cat yyy zzz > xxx
$ notes -a xxx yyy zzz

# example 2:
$ cat ~/.notesrc | notes -a! notesrc -
```

#### -a[!]+, --append[!]
Same as `-a` but instead of overwriting, the new note is appended to the file.
If the name does not exist, then an error will be issued;
use `!` option to create it,
or set the clobber variable to _false_ in the configuration file.

#### -v, --view
Shows the _note_ with the default **$PAGER** if one is not specified in the configuration file.

```
$ cat -v about-groff
```

#### -p, --print
Same as `-v` but writes the contents to **stdout**.

#### -e, --edit
Loads the _note_ to the default **$EDITOR** if one is not specified in the configuration file.
Also, it can be used with `--add/--append` if it is next to it.

#### -l, --list
Displays the notes names that match _pattern_.

#### -f, --files
Same as `-l` but prints out the _full-path filenames_.

#### -d, --delete
Deletes a note.

#### -r, --rename
Renames and/or moves a note. A second parameter is required to specify the new
name. If file extension is specified in the new name, then it will use it.
_rename_ can also change the section if separated by '/' before the name,
e.g., `section3/new-name`.

#### -a, --all
Displays all notes that were found; it works together with `-v`, `-p`, `-e`, and `-d`.
Do not use it as first option because it means `--add`.

#### -h, --help
Displays a short help text and exits.

#### -c, --rcfile
Read this _rcfile_ for setting notes options, instead of reading
the default user's _notesrc_ file.

#### --version
Displays the program version, copyright and license information and exits.

#### --onstart
Executes the command defined by `onstart` in the configuration file
and returns its exit code.
This option is useful when custom synchronization is needed.

#### --onexit
Executes the command defined by `onexit` in the configuration file
and returns its exit code.
This option is useful when custom synchronization is needed.

## ENVIRONMENT
The **SHELL**, **EDITOR** and **PAGER** environment variables are used.

#### NOTESDIR
The directory of notes.
Usually used to pass this information to shell.

#### NOTESFILES
The list of selected files to process.
Usually used to pass this information to shell.

#### NOTESPAGER
If set, the default pager for notes.

#### NOTESEDITOR
If set, the default editor for notes.

#### BACKUPDIR
If set, the default backup directory.

## FILES
When `--rcfile` is given,
**notes** will read the specified file for setting its options and key bindings.
Without that option, **notes** will read the user's _notesrc_ (if it exists), 
either `$XDG_CONFIG_HOME/notes/notesrc` or `~/.config/notes/notesrc`
or `~/.notesrc`, whichever is encountered first.
See [notesrc 5](man).

## COPYRIGHT
Copyright © 2020-2022 Nicholas Christopoulos.

License GPLv3+: GNU GPL version 3 or later [https://gnu.org/licenses/gpl.html](https://gnu.org/licenses/gpl.html).
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

## HOMEPAGE
[Homepage](https://codeberg.org/nereusx/notes).

## AUTHOR
Written by Nicholas Christopoulos [nereus@freemail.gr](nereus@freemail.gr).

## SEE ALSO
[notesrc 5](man).

