# databaseBuild
A Relational Bottom-up Layered Database Internal Components Build

1. Constructed a record-based file manager (RBFM) to manage records and pages in files and a catalog-based relation manager (RM) for maintaining catalogs/indexes files. 
2. Implemented an index manager (IX) based on B+ Tree structure to create indexes over unordered records stored in heap file. 
3. Built a query engine (QE) supporting relational operators like filtering, projection, join (nested-block join, nested-index join, grace hash join) and aggregation. 
4. Database performance reached 1,000,000 records insertion within 5 minutes.

# Record based file manager

* record format:

``` bash
	|————————————————————————————————————————————————————————————————————————|
	| RID | each relative field offset | every attribute value(without null) |
	|————————————————————————————————————————————————————————————————————————|
```

This format can realize O(1) access in the several actions.

* category support:

TypeInt (int), TypeReal (float), TypeVarChar (String)

* Page Format:

page default: store signature(sign), three counters in the beginning.
page 0~ : store record

``` bash
	————————————————————————————————
	|   Records                   |
	|                             |
	|                             |
	|                             |
	|                             |
	|                             |
	|                             |
	|                             |
	|                             |
	|_____________________________|
	|          |slot |Slot   |Left|
	|__________|table|Number_|Size|
```

* Internal Record Format

The record bit representation format is:

``` bash
	--------------------------------------------------------------------------------------------------
	|2 bytes|2 bytes|2 bytes|...|2 bytes| Attribute 1 Data | Attribute 2 Data |...| Attribute N Data |
	--------------------------------------------------------------------------------------------------

	|---- N Short Int Type Pointers ----|----------------- N Attribute Information ------------------|
```

* Page Format

``` bash
   ---------------------> record are inserted accordingly
       ---------------------------------------------------------------
       |          |          |                                       |
       | record 1 | record 2 | ... ...                               |
       |          |          |                                       |
       |-------------------------------------------------------------|
       |                                                             |
       |                                                             |
       |                                                             |
       |                                                             |
       |                                                             |
       |                                                             |
       |                                                             |
       |                           ......                            |
       |                                                             |
       |                                                             |
       |                                                             |
       |                        slots are inserted reversely         |
       |                 <------------------------------------------ |
       |                 ____________________________________________|
       |                |        |        |        |        |        |
       |                |  ....  | Slot # | Length |   N    |   F    |
       |                |        |        |        |        |        |
       ---------------------------------------------------------------

			     |---- Slot 0 -----|
		    |---------------- Directory -----------------|
```

* update & delete


# Relational Maneger 

* file format

``` bash
       ---------------------------------------------------------------
       |sign 0 0 0 1                                                 |
       |                             			             |  <-------- "Virtual first page"
       |                                                             |
       |-------------------------------------------------------------|
       |(1, "Tables", "Tables")(2, "Columns", "Columns")             |
       |                                                             |
       |                                                             |
       |                                                             |  <-------- Page 0
       |                                                             |
       |                                                             |
       |                 ____________________________________________|
       |                |        |        |        |        |        |
       |                |  ....  | Slot # | Length |   N    |   F    |
       |                |        |        |        |        |        |
       ---------------------------------------------------------------
       |                                                             |
       |                                                             |
       |                                                             |
       |                                                             |  <-- Other pages (will be appended as the records inserted)
       |                                                             |
       |                           ......                            |
       |                                                             |
       |                                                             |
       |                                                             |
```

# Index Manager (B+ tree)

* Index Entry Format

For non-leaf node

``` bash

	   -------------------------------------------------
	   |    4 bytes    |      key      |    4 bytes    |     	
	   -------------------------------------------------

	   |- page number -|------key------|- page number -|
```

for the leaf node

``` bash
		   ---------------------------------
		   |      key      |    4 bytes    |     	
		   ---------------------------------

		   |------key------|------RID------|
```

* Page format

``` bash

				     ---------------------> record are inserted accordingly
	       ---------------------------------------------------------------
	       |                     |          |          |                 |
	       |     Header          | PageNum1 |   Key 1  |   PageNum2      |
	       |                     |          |          |                 |
	       |-------------------------------------------------------------|
	       |                                  |                          |
	       |       ......           ......    |                          |
	       |                                  |                          |
	       |----------------------------------                           |
	       |                                                             |
	       |                                                             |
	       |                                                             |
	       |                           ......                            |
	       |                                                             |
	       |                                                             |
	       |                                                             |
	       |                        offsets are inserted reversely       |
	       |                 <-------------------------------------------|
	       |                 ____________________________________________|
	       |                |        |        |        |        |        |
	       |                |  ....  |offset 3|offset 2|offset 1| offset0|
	       |                |        |        |        |        |        |
	       ---------------------------------------------------------------


				|----------------- Directory ----------------|
```

* leaf-page (leaf node) design

``` bash

				     ---------------------> record are inserted accordingly
	       ---------------------------------------------------------------
	       |                     |          |          |        |        |
	       |     Header          |   Key1   |    RID1  |   Key2 |  RID2  |
	       |                     |          |          |        |        |
	       |-------------------------------------------------------------|
	       |                                  |                          |
	       |       ......           ......    |                          |
	       |                                  |                          |
	       |----------------------------------                           |
	       |                                                             |
	       |                                                             |
	       |                                                             |
	       |                           ......                            |
	       |                                                             |
	       |                                                             |
	       |                                                             |
	       |                        offsets are inserted reversely       |
	       |                 <----------------------------------         |
	       |                 ____________________________________________|
	       |                |        |        |        |        |        |
	       |                |  ....  |offset 2|offset 1|offset 0|NextPage|
	       |                |        |        |        |        |        |
	       ---------------------------------------------------------------


				|----------------- Directory -------|
```

* B+ tree printing
<a href="https://alleninwood.github.io/2017/11/21/The-implementation-of-B-tree-printing/">link</a>

# Query Engine
* Catalog extension information
``` bash
	   -------------------------------------------------------------
	   |    id    |     indexTablesName     |    indexFilesName    |     	
	   -------------------------------------------------------------
```

* Block Nested Loop Join
``` bash
		      number of pages * page size byte
	   -------------------------------------------------------------
	   |                                                           | 
	   |  retrieve until buffer is full                            | 
leftTable ---------------------------------->                          | 
	   |                                                           |                   |---------------------------| flush to disk
	   |                build a in memory hash map                 | 		   |         		   --------------------->
	   |                                                           |   join		   |	output buffer          |
	   |                                                           |-----------------------> 		       |
	   |                                                           |                   ----------------------------- 	
	   -------------------------------------------------------------
		/\
		|
		|
		| do a look up each record
		|
		|
	    rightTable
```

* Index Nested Loop Join

* Grace Hash Join

* Aggregation
1. Basic aggregation
2. Group-based hash aggregation
