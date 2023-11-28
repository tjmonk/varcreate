# varcreate
Create variables for the varserver from a JSON file

## Prerequisites

Varcreate requires a running varserver (https://github.com/tjmonk/varserver)

## Overview

Varcreate consists of a utility and a libvarcreate shared object library.
The varcreate utility provides a means to quickly create variables for the varserver from a JSON configuration file.  The core functionality of the varcreate utility is implemented in the libvarcreate shared object library.  It has been implemented this way to allow you to easily integrate variable creation from a configuration file into your own applications which are built around the variable server.

## Configuration File Format

The varcreate configuration file consists of an array of JSON objects where each JSON object describes a single variable.  Every attribute of a varserver variable can be configured.  Attributes include:

- name
- guid
- type
- format specifier
- tags
- initial value
- short name
- description
- flags
- alias
- read/write permissions
- length ( for string variables )

The minimum required set of attributes is:

- name
- type

## Example configuration file

```
{
	"type":"vars",
	"version":"1.0",
	"description":"Configuration Vars",
	"modified":"23-May-2023",
	"vars":
	[
		{
			"name":"/SYS/TEST/A",
			"guid":"0x80000",
			"type":"uint16",
			"tags":"sys,test,foo,sys:test",
			"fmt":"%2d",
			"value":"10",
			"shortname":"TestA",
			"description":"Test value A",
			"flags":"volatile",
			"read":"1000,1001",
			"write":"1000"
	    },
		{
			"name":"/SYS/TEST/B",
			"guid":"0x80001",
			"type":"uint32",
			"fmt":"%02u",
			"tags":"sys,test,foo,sys:test",
			"value":"12",
			"shortname":"TestB",
			"description":"Test value B",
			"flags":"volatile",
			"read":"1000,1001",
			"write":"1000"
		},
		{
			"name":"/SYS/TEST/C",
			"guid":"0x80002",
			"type":"str",
			"length":"128",
			"fmt":"%-10s",
			"tags":"sys,test,foo,sys:test",
			"value":"This is a test",
			"shortname":"TestC",
			"description":"Test value C",
			"flags":"volatile",
			"read":"1000,1001",
			"write":"1000"
		},
		{
			"name":"/SYS/TEST/F",
			"guid":"0x80002",
			"type":"float",
			"fmt":"%0.2f",
			"tags":"sys,test,foo,sys:test",
			"value":"23.45",
			"shortname":"TestF",
			"description":"Test value F",
			"flags":"volatile",
			"read":"1000,1001",
			"write":"1000"
		},
		{
			"name":"/SYS/TEST/DELAY",
			"guid":"0x80002",
			"type":"uint16",
			"tags":"sys,test,foo,sys:test",
			"value":"100",
			"shortname":"Delay",
			"description":"Delay value",
			"read":"1000,1001",
			"write":"1000"
		}
	]
}

```

## Aliasing

Aliasing refers to the creation of one or more variables which are alternate
names for an existing variable.

The alias attribute can either be a string,

e.g. "alias" : "varname"

or an array of strings in the case where more than one alias is required

e.g. "alias" : ["varname1","varname2","varname3"]

Aliases are created after the primary variable has been created.

Aliases are first class citizens and indistinguishable in behavior from the
variable they are aliasing.  You can even create aliases of aliases if you
desire.

Aliased variables (and their aliases) automatically have the "alias" flag
assigned to them.

## Build

```
./build.sh
```

## Test

### Creating variables from a single file

Start the varserver if it is not already running

```
varserver &
```

Create variables from the test configuration file

```
varcreate varcreate/test/vars.json
```

Show the created variables and their values

```
vars -v
```

### Create Variables from all files in a directory

Start the varserver if it is not already running

```
varserver &
```

Create variables from the test configuration directory

```
varcreate -d -v varcreate/test
```

Show the created variables and their values

```
vars -v
```

Alternatively, the varcreate utility can create multiple
## Acknowledgements

The libvarcreate library utilizes the cJSON source from Dave Gamble ( https://github.com/DaveGamble/cJSON )


