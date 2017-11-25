* Record based file manager
** record format:

``` bash
|————————————————————————————————————————————————————————————————————————|
| RID | each relative field offset | every attribute value(without null) |
|————————————————————————————————————————————————————————————————————————|
```

This format can realize O(1) access in the following action:
(1)find the exact page according to former half of RID.
(2)find the exact slot table(offset & length) according to latter half of RID.
(3)access exact record according to founded slot table.

** category support:

This relational database support three kind categories storing: TypeInt (int), TypeReal (float), TypeVarChar (String)
Store the VarChar value in “every attribute value (without null)” section without length.
Can obtain the length of VarChar through the difference between relative offset.

** Page Format:

page default: store signature(1088), three counters in the beginning.
page 0~ : store record
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

** Meta-data

The file Tables records all the record files information, including itself. 
Each record in the file "Tables" has three attributes: table-id (INT type), table-name (VARCHAR type), and file-name (VARCHAR type).
The file Columns records all the attributes information of each file, including itself. 
Each record in the file "Columns" has five attributes: table-id (INT type), column-name (VARCHAR type), column-type (INT type), column-length (INT type), column-position (INT type).
Here, the value of table-id starts from 1, and will increase every time a new table is created. 
So when a new catalog is created, the records with table-name "Tables" has id = 1; the records with table-name "Columns" has id = 2.
The records stored in Tables and Columns follow the format that is shown in the next section.

** Internal Record Format

The record bit representation format is:

``` bash
--------------------------------------------------------------------------------------------------
|2 bytes|2 bytes|2 bytes|...|2 bytes| Attribute 1 Data | Attribute 2 Data |...| Attribute N Data |
--------------------------------------------------------------------------------------------------

|---- N Short Int Type Pointers ----|----------------- N Attribute Information ------------------|
```

At the beginning of each record, there are N pointers. Every pointer is short int type and record the offset information of each attribute in the whole record.
The pointer points to the head of each attribute, which means each 2-byte slot records the relstive position number of each attribute, i.e., the offset to the head of the record.
For example, the first attibute information starts at the 50th byte (50 byte offset to the head of the record), then the first pointer storage number 50 as the short int type.
The length of each attribute data is gained by the difference of its next adjacent non-NULL pointer values and its pointer values. 
If the second pointer value is 60, then it means the first attribute information has 10 bytes.

To storage NULL attribute information, I use -1 as the corresponding attribute pointer value. 
The NULL attribute information will be skipped (NULL information) in the attribute data section, but won`t be skipped at the pointer section.
So if there are totally N attributes, and M of them are NULL, then the record will still have N pointers at the head section, and (N-M) attribute information in the attribute data section.

To access a certain attribute, just read its pointer value p1 and its next non-NULL pointer value p2, and then read the record data block start from offset p1 and has length (p2-p1).
If all the attribute information after this wanted attribute are NULL, then p2 is the end of the whole record, which could be calculated based on the given RID.
Thus, this bit representation format satisfies O(1)field access.

** Page Format
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

Each page contains record data and directory. The right-most directory information is marked as "F" above, which is the flag, recording the free space in bytes of this page.
The next value in the directory is "N", which is the number of the slots. Both "F" and "N" are 2 bytes, storaged in short int type.
Then, from the left-most bit of "N" block, the other left slots in the directory are slots for records. Each record slots are 4 bytes, containing 2 components: slot number and record length.
They are also short int type. The slot number records the offset value of the head of each record inside this page, which is the offset byte within this page (i.e., 0 <= offset < 4096).

The record are storaged one by one without empty bytes in between, and their slots are storaged reversely from the end to the head of the page.
Both the slot number and the page number are from 0 instead of 1.

** update & delete
When delete a record, we first read the page of that record residents in, and find its slot according to the given RID. We first check whether this record has been deleted and whether the record is a pointer (which means this record has been updated and re-inserted into another page). If it has been deleted, then return -1. If it is a pointer, then we update the slot value in this page to be invalid, and go to the pointed page to repeat the operation until we find the corresponding record data. After that, we read out the head offset and the length of the record, and set the slot to be invalid (by setting the value of the slot -1). And then we move the later records forward to fill out the "hole".

When update a record, we first compare the length of the original record and the updated record. If the new length is smaller than the original one, then the later records will be moved forward, and the slot values of the later records will be updated; if the new length is equal to the original one, then there is no change of the directory. If the new length is larger than the original one, and the original page could hold this new record, then the later records should be moved backward, and thus the slot values of them will be updated. However, if the original page could not hold the original
record, then the original record will become a 8 byte pointer, which indicates the new place of the record. The pointer`s format is as below:

``` bash
		_____________________________________________________
                                    	|         |                     |                   |
                                    	|   -2    |   New Page Number   |  New Slot Number  |
		|_________|_____________________|___________________| 
		|---------|---------------------|-------------------|
                                      	  2 bytes         4 bytes              2 bytes
```

** File Format

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
	       |                                                             |  <--------- Other pages (will be appended as the records inserted)
	       |                                                             |
	       |                           ......                            |
	       |                                                             |
	       |                                                             |
	       |                                                             |
```

In the pointer, -2 is the flag indicating this record is actually a pointer. The page number is int type with 4 bytes, while the slot number is 2 bytes.
* Relational Manager

