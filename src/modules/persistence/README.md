# Persistence layer

## Purpose

This module manages the persistence layer and provides drivers to talk to the database. Currently the only driver implemented is for postgresql.

The main class for doing database interaction is the `DBHandler`.

In order to generate models that represent the tables, you can use the `databasetool` to generate the models from metadata files.

A more high level class to manage updates is the `PersistenceMgr`. It collects dirty-marked models and performs a mass-delta-update via prepared statements. You should use this for e.g. player updates.

It's always a good idea to check out the unit tests to get an idea of the functionality of those classes.

## Usage DBHandler

You have to create the models that are usually put into the namespace `db`. Once you have those models, you can use the generated getters and setters to prepare the model. This can now get send over to the `DBHandler`. There are methods to insert, update, delete or select particular entries from tables via the model values.

Copying the Model class instance is no problem, it's fast. The stuff that is copied is only 32bytes at the moment.

### Create (and update) table

```cpp
_dbHandler->createOrUpdateTable(db::EventModel());
```

The persistence layer has automatic upgrading and downgrading support for your tables. By defining them in a `.tbl` file, the system is able to generate the needed `ALTER` statements to get to your desired state.

Workflow to update a table:

- Edit the `tbl` file
- Run your build
- Run the application
  - The table, constraint, sequence... is updated to the state that is defined in your code. This allows you to go back and forth in your commits to test things. ***But keep in mind that removing a column from a table can lead to data loss. Because re-adding it, doesn't remember the previous values of course.***

### Select by condition

```cpp
_dbHandler->select(db::EventModel(), persistence::DBConditionOne(), [this] (db::EventModel&& model) {
  [...]
});
```

### Multiple search conditions

```cpp
const db::DBConditionTestModelEmail emailCond("a@b.c");
const db::DBConditionTestModelName nameCond("Foo Bar");
_dbHandler->select(db::TestModel(), persistence::DBConditionMultiple(true, {&emailCond, &nameCond})), [this] (db::TestModel&& model) {
  [...]
});
```
