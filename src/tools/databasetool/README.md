# Databasetool

## Table descriptions for the databasetool

The databasetool binary will generate model files for the
table definitions given in `*.tbl` files. You can specify the fields,
the constraints and set an operator for dealing with conflict states (more
about that later).

To add a new `*.tbl` file to a module and automatically generate code
for that table definition, you have to add the following after your
cmake `add_library(${LIB} ${SRCS})` call:

```cmake
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl ExampleModels.h)
```

`ExampleModels.h` specifies a single header where all generated table models
are put into.

The generated models can be used with the `DBHandler` from the `persistence` module.

## Example

If no classname is specified, the table name will be used with `Model` as postfix.

Table `user` will be generated as `UserModel` class, if no other `classname` was
specified

All models will be put into a `db` namespace - even if you specify your own namespace. For more details, see the parameter description below.

```c
table <TABLENAME> {
  classname <STRING> (overrides the automatically determined name)
  namespace <STRING> (c++ namespace where the class is put into)
  schema <STRING> (default is public)
  field <FIELDNAME> {
    type <FIELDTYPE> (default: string)
    notnull (optional)
    length <LENGTH> (optional)
    operator <OPERATOR> (default: set)
    lowercase (optional)
    default <DEFAULTVALUE> (optional)
  }
  constraints {
    <FIELDNAME> unique
    <FIELDNAME> index
    <FIELDNAME> primarykey
    <FIELDNAME2> primarykey
    <FIELDNAME> autoincrement
    (<FIELD1>, <FIELD2>) unique
    <FIELDNAME> foreignkey <FOREIGNTABLE> <FOREIGNFIELD>
  }
}
```

## table

A definition starts with `table <TABLENAME>`. The body is enclosed by `{` and `}`.

### classname

This can be used to override the auto generated class name. The auto generated class name is generated from the table name converted to UpperCamelCase. This converts a table name like `my_table` to `MyTable` or `mytable` to `Mytable`.

### namespace

You specify a namespace in your table definition that is called `mynamespace`. The table is called `MyTable`. The resulting c++ class will live in `mynamespace::db::MyTable`. If you omit the namespace setting in your table definition, the class will live in `db::MyTable`.

### schema

Specifies the schema name that should be used for the table.

### field

A field describes a table column, the name, the type and so on. This block is enclosed by `{` and `}`.

#### default

The default value for the field.

#### length

Specifies the optional length of the field.

#### notnull

If this is specified, the field may not be null.

#### operator

The operator is taken into account when you execute an insert or
update statement and hit a unique key violation.

This can e.g. be used to increase or decrease points for particular keys.
The first time you normally perform an insert - and the following times
you will hit a key violation and thus perform the insert or update with
the operator specified. The default operator is `set`. See a full list
of valid operators below.

##### Valid operators

* `set`
* `add`
* `subtract`

#### lowercase

Convert a string value to lowercase before entering it into the database. This may not be set for `password` types of course.

#### type

Valid field types

* `password`
* `string`
* `text`
* `int`
* `long`
* `timestamp`
* `boolean`
* `short`
* `byte`
* `blob`

### constraints

Here you can specify foreign key constraints, auto increment values and so on. This block is enclosed by `{` and `}`.

See the example above for a list of supported constraints.

## Other notable features

* Timestamps are handled in UTC.
* When using `int` or `short` as a field type, there is also a setter configured that accepts enums.
