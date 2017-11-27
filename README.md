# databaseBuild
Build a bottom-up relational database

details below:

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
	       |                             			                           |  <-------- "Virtual first page"
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
