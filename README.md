# ProgettoAPI2020

Description of the project can be found [here](https://github.com/pablogiaccaglia/Progetto-API-2020/blob/main/Specific%20Documents/ProvaFinale2020.pdf).

Specific document can be found [here](https://github.com/pablogiaccaglia/Progetto-API-2020/blob/main/Specific%20Documents/ProvaFinale2020specifica.pdf).

## edU, or ed multiplies Undo 
*This project has been developed as part of the "Algoritmi e Principi dell'Informatica" course at [Politecnico di Milano](https://www.polimi.it/).* It has been evaluated "30/30L".

A C program that represents a text editor, where you can add, remove, print lines and do undo or redo actions.

### Input Format
The program expects its input from stdio with the following format.

_**NOTE**_: MAX length of a line: 1024. Commands are considered correct.
#### Addition of lines
To add a line to the editor, it is needed to write the lines after the ```ind1,ind2c``` instruction, and after the last line, write a '.'.

Example:
```
1,2c
First Line
Second Line
.

```

#### Deletion of a line
To delete lines, it is needed to use the following format:
```ind1,ind2d```
_**NOTE**_: if you try to delete lines that doesn't exists, the command will have no effect.

#### Printing lines
To print a group of lines, it is needed to use the following format:
```
ind1,ind2p
```
_**NOTE**_: if the line doesn't exists, it will print a '.'.

#### Undo action
In order to go back to a previous version, you can do the following command 
``ind1u``
where ind1 is the number of version that you want to go back.

#### Redo Action
Like undo, the command to do a redo is the following one:
``ind1r``

***Example of the input stream:***
 ```
1,2c
first line
second line
.
2,3c
new second line
terza riga
.
1,3p
1,1c
new first line.
1,2p
2,2d
4,5p
1,3p
4,5d
1,4p
3u
1,6p
1r
1,3p
q

 ```

***Example of the output stream:***
 ```
 first line
 new second line
 third line
 new first line
 new second line
 .
 .
 new first line
 third line
 .
 new first line
 third line
 .
 .
 first line
 new second line
 third line
 .
 .
 .
 new first line
 new second line
 third line
 ```

### Testing Result

<img src="https://github.com/pablogiaccaglia/Progetto-API-2020/blob/main/performance.jpeg"  px align="center" />

