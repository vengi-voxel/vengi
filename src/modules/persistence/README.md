# Purpose

This module manages the persistence layer and provides (potentially more than the one that exists) drivers to talk to the database.

The main class for doing database interaction is the `DBHandler`.

In order to generate models that represent the tables, you can use the `databasetool` to generate the models from metadata files.

A more high level class to manage updates is the `PersistenceMgr`. It collects dirty-marked models and performs a mass-delta-update via
prepared statements. You should use this for e.g. player updates.

It's always a good idea to check out the unit tests to get an idea of the functionality of those classes.

# Usage DBHandler

You have to create the models that are usually put into the namespace `db`. Once you have those models, you can use the generated getters and
setters to prepare the model. This can now get send over to the `DBHandler`. There are methods to insert, update, delete or select particular
entries from tables via the model values.

## Create (and update) table

```
  _dbHandler->createTable(db::EventModel());
```

## Select by condition

```
  _dbHandler->select(db::EventModel(), persistence::DBConditionOne(), [this] (db::EventModel&& model) {
    [...]
  });
```
