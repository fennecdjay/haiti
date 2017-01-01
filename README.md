Haiti
=====

******
stands for  

Help Annotation and Issue Tracking Inline
******

inspired by [watson](http://goosecode.com/watson/) (but does not handle github issues, at least for now).


configure
---------
edit config.mk to your liking

build
-----
in the source directory  

```
make
```

of course, you can also

```
make clean
```

hacking
-------
  * add types: look in filetype.h
  * ...  

install
-------
not ready yet.  
but it will be something like
  
```
make install
```

usage
-----
*Haiti* detects special comments in your source code. 

create a [tagged] comment:

```
// [mytag] this comment has key 'mytag'
```

handle checkboxes:

```
// [ ] empty    checkbox  (TODO)
// [S] skipped  checkbox  (SKIP)
// [*] running  checkbox  (TEST)
// [X] finished checkbox  [DONE)
```

there can be a tag with a checkbox:

```
// [mytag] [*] means: working on `mytag`
```

comment last tag:

```
// [mytag] a comment
// | this comment adds to the previous
// | also this one
// || this one comments the last comment
```
example
-------
parse what's in **haiti.c**:

```
./haiti haiti.c
```

parse what's in current directory

```
./haiti .
```

parse what's in **haiti.c** and show only tag `SHOWME`

```
./haiti haiti.c SHOWME
```

or

```
./haiti SHOWME haiti.c
```	


### TODO
adds screenshots or similar.
with *what?*  




Handling issues
---------------
create a comment like:

```
// #? [label] creates new issue with label 'label'

```

run:

```sh
./haiti -sync myfile
```

after, it will look like:

```
// #n [label] creates new issue with label 'label'
```

where *n* is the created issue number.
you can then close it with:

```
// %n [label] creates new issue with label 'label' 
```


Issues  
------
[here](https://github.com/fennecdjay/haiti/issues) please.

that about all.
