#include <stdio.h>
#include <stdlib.h>
#include<string.h>

/**********************************************************
                        CONSTANTS
**********************************************************/
#define INPUT_SIZE 1000000000
#define ALLOC_INCREMENT 500

/**********************************************************
                        COMMAND CHARACTERS
**********************************************************/
#define QUIT_C 'q'   /* quit character */
#define CHANGE_C 'c' /* change character */
#define DELETE_C 'd' /* delete character */
#define PRINT_C 'p'  /* print character */
#define UNDO_C 'u'   /* undo character */
#define REDO_C 'r'   /* redo character */

/**********************************************************
                        GLOBAL VARIABLES
**********************************************************/

int lastGlobalIndex = 0,start = 0, end = 0, undosToExecute = 0, undosToAdd = 0, redosToExecute = 0, redosToAdd = 0;
char **indexes;
char *currentCommand;
char *inputBuffer;
char firstCharInInputCommand, secondCharInInputCommand;

/**********************************************************
                        DATA TYPES
**********************************************************/

struct command
{
    int fin;
    char commandIdentifier;
    char **stringsBeforeCommand;
    char **stringsAfterCommand;
    int firstIndex,lastIndex,commandFlag; // commandFlag -> se il primo indice è dopo l'ultimo indice già presente --> = 1 , non serve gestire restore;
    struct command *nextCommand;
};
struct command *topUndo = NULL;
struct command *topRedo = NULL;

/**********************************************************
                        REDO FUNCTIONS
**********************************************************/

void pushToRedoStack(struct command **input) {
    struct command *temp = *input;
    temp->nextCommand = topRedo;
    topRedo = temp;
}

//moves redo to undo stack
struct command* popRedo(){
    struct command *temp = topRedo;
    topRedo = topRedo->nextCommand;
    temp->nextCommand = topUndo;
    topUndo = temp;
    return temp;
}

void redoDelete(struct command *commandToRedo)
{
    commandToRedo->stringsBeforeCommand = indexes;
    indexes= commandToRedo->stringsAfterCommand;
    lastGlobalIndex = commandToRedo->lastIndex;
}

void redoChange(struct command* commandToRedo)
{
    if(commandToRedo->commandFlag)
        lastGlobalIndex = commandToRedo->lastIndex;
    else {
        indexes = commandToRedo->stringsAfterCommand;
        lastGlobalIndex = commandToRedo->lastIndex;
    }
}

void redoAction(int numberOfRedosToExecute){

    struct command* pop;
    if(numberOfRedosToExecute>=redosToAdd)
        numberOfRedosToExecute = redosToAdd;

    for (int i = 0 ; i<numberOfRedosToExecute; i++)
    {
        pop = popRedo();
        if(pop->commandIdentifier == CHANGE_C)
            redoChange(pop);
        else if(pop->commandIdentifier == DELETE_C && pop->fin != 0)
            redoDelete(pop);
    }
    redosToAdd = redosToAdd - numberOfRedosToExecute;
    undosToAdd = undosToAdd + numberOfRedosToExecute;
}

/**********************************************************
                        UNDO FUNCTIONS
**********************************************************/

void pushToUndoStack(int lastIndex, char commandIdentifier)
{
    struct command *undoCommand;
    undoCommand=malloc(sizeof(struct command));
    undoCommand->commandIdentifier = commandIdentifier;
    undoCommand->fin = lastIndex;
    undoCommand->commandFlag = 0;
    undoCommand->firstIndex = lastGlobalIndex;
    undoCommand->nextCommand = topUndo;
    topUndo = undoCommand;
}

struct command* popUndo(){
    struct command *temp = topUndo;
    topUndo = topUndo->nextCommand;
    return temp;
}

void undoChange(struct command* commandToUndo)
{
    if(commandToUndo->commandFlag)
        lastGlobalIndex = commandToUndo->firstIndex;
    else{
        commandToUndo->stringsAfterCommand = indexes;
        indexes = commandToUndo->stringsBeforeCommand;
        lastGlobalIndex =commandToUndo->firstIndex;
    }
}

void undoDelete(struct command *commandToUndo)
{
    commandToUndo->stringsAfterCommand = indexes;
    indexes = commandToUndo->stringsBeforeCommand;
    lastGlobalIndex = commandToUndo->firstIndex;

}

void undoAction(int numberOfUndosToExecute, int hasPrintBeenRequested){

    struct command* pop;
    if(numberOfUndosToExecute>undosToAdd)
        numberOfUndosToExecute = undosToAdd;
    for (int i = 0 ; i<numberOfUndosToExecute; i++)
    {
        pop = popUndo();
        if (pop->commandIdentifier == CHANGE_C)
            undoChange(pop);
        else if(pop->commandIdentifier == DELETE_C && pop->fin!= 0)
            undoDelete(pop);
        if(hasPrintBeenRequested)
            pushToRedoStack(&pop);
    }
    undosToAdd = undosToAdd - numberOfUndosToExecute;
    redosToAdd = redosToAdd + numberOfUndosToExecute;

}

/**********************************************************
                     EDITOR COMMANDS FUNCTIONS
**********************************************************/

void print(int firstIndex, int lastIndex)
{
    int charPosition;
    if (lastGlobalIndex > 0) {
        for (int i = firstIndex; i <= lastIndex; i++) {
            if (i > 0 && i <= lastGlobalIndex) {
                charPosition=0;
                while(indexes[i - 1][charPosition] != '\0')
                {
                    putc_unlocked(indexes[i - 1][charPosition], stdout);
                    charPosition++;
                }
                putc_unlocked('\n',stdout);
            } else
                fputs(".\n", stdout);
        }

    } else
        for (int i = firstIndex; i <= lastIndex; i++)
            fputs(".\n", stdout);
}

void change(int firstIndex, int lastIndex) {
    int currentIndex,k = 0;
    int newLastGlobalIndex = lastGlobalIndex;
    pushToUndoStack(lastIndex, CHANGE_C);
    int numberOfLinesToAlter = lastIndex - firstIndex + 1;
    if(lastIndex>lastGlobalIndex) {
        newLastGlobalIndex = lastIndex;
        indexes = realloc(indexes, (sizeof indexes * (lastGlobalIndex + ALLOC_INCREMENT)));
    }
    if(firstIndex>lastGlobalIndex)
    {
        topUndo->commandFlag = 1;
        for(currentIndex=firstIndex; currentIndex<=lastIndex; currentIndex++)
            indexes[currentIndex - 1] = strtok(NULL, "\n");
        topUndo->firstIndex = lastGlobalIndex;
        topUndo->lastIndex = newLastGlobalIndex; //indice dopo la chang
    }
    else {
        topUndo->stringsBeforeCommand = malloc((numberOfLinesToAlter+lastGlobalIndex)*sizeof(char *));
        for (currentIndex = 1; currentIndex <= newLastGlobalIndex; currentIndex++) {
            if ( currentIndex >= firstIndex && currentIndex <= lastIndex ) {
                topUndo->stringsBeforeCommand[k] = indexes[currentIndex - 1];
                indexes[currentIndex - 1] = strtok(NULL, "\n");
            } else
                topUndo->stringsBeforeCommand[k] = indexes[currentIndex - 1];
            k++;
        }
        topUndo->firstIndex = lastGlobalIndex;
        topUndo->lastIndex = newLastGlobalIndex; //indice dopo la change
    }
    lastGlobalIndex = newLastGlobalIndex;
    strtok(NULL, "\n");
}

void delete(int firstIndex, int lastIndex) {

    if (firstIndex == 0)
        firstIndex = 1;// esegue se è una istruzione d da terminale

    int numberOfLinesToDelete,i = firstIndex, j=0, k;

    pushToUndoStack(lastIndex, DELETE_C);

    if (firstIndex <= lastGlobalIndex && lastIndex != 0 ) {
        if(lastIndex > lastGlobalIndex)
            lastIndex = lastGlobalIndex;
        numberOfLinesToDelete = lastIndex - firstIndex + 1;
        topUndo->stringsBeforeCommand = malloc((lastGlobalIndex + 1) * sizeof(char *));
        if (lastIndex >= lastGlobalIndex ) {
            for(k = 0; k<=lastGlobalIndex; k++)
                topUndo->stringsBeforeCommand[k] = indexes[k];
            lastGlobalIndex = firstIndex - 1;
            topUndo->commandFlag = 0;
            topUndo->lastIndex = lastGlobalIndex;
        } else if (lastIndex < lastGlobalIndex ) {
            if(firstIndex != 1)
                for (k = 0; k < firstIndex - 1; k++) {
                    topUndo->stringsBeforeCommand[j] = indexes[k];
                    j++;
                }

            while (i <= lastIndex ) {
                topUndo->stringsBeforeCommand[j] = indexes[i - 1];
                j++;
                i++;
            }
            while (i <= lastGlobalIndex) {
                topUndo->stringsBeforeCommand[j] = indexes[i - 1];
                indexes[i - numberOfLinesToDelete - 1] = indexes[i - 1];
                i++;
                j++;
            }
            if (firstIndex < lastGlobalIndex)
                lastGlobalIndex = lastGlobalIndex - numberOfLinesToDelete;

            topUndo->commandFlag = 0;
            topUndo->lastIndex = lastGlobalIndex;
        }
    } else
        topUndo->lastIndex = 0;
}

void executeUndoRedo(int hasPrintBeenRequested)
{
    int numberOfActionsToExecute = undosToExecute-redosToExecute;
    if(numberOfActionsToExecute == 0)
        return;
    if(numberOfActionsToExecute>0 && topUndo!=NULL)
        undoAction(numberOfActionsToExecute, hasPrintBeenRequested);
    else if(numberOfActionsToExecute<0 && topRedo!=NULL)  {
        numberOfActionsToExecute = -numberOfActionsToExecute;
        redoAction(numberOfActionsToExecute);
    }
}

/**********************************************************
                     PARSING FUNCTIONS
**********************************************************/

int parseInputWithTracking(){

    while(1) {
        currentCommand = strtok(NULL, "\n");
        if(currentCommand == NULL || currentCommand[0] == QUIT_C)
            return 1;
        sscanf(currentCommand, "%d%c%d%c ", &start , &firstCharInInputCommand , &end , &secondCharInInputCommand);
        if (firstCharInInputCommand == UNDO_C){
            undosToExecute = undosToExecute + start;
            if(undosToExecute>undosToAdd+redosToExecute)
                undosToExecute = undosToAdd+redosToExecute;
        }
        else if (firstCharInInputCommand == REDO_C){
            redosToExecute = redosToExecute + start;
            if(redosToExecute>(redosToAdd+undosToExecute) )
                redosToExecute = (redosToAdd+undosToExecute);
        }
        else {
            if (secondCharInInputCommand == CHANGE_C) {
                executeUndoRedo(0);
                if(redosToAdd>0) {
                    topRedo=NULL;
                    redosToAdd = 0;
                }
                undosToAdd = undosToAdd +1;
                change(start, end);
            } else if (secondCharInInputCommand == PRINT_C) {
                executeUndoRedo(1);
                print(start, end);
            }
            else {
                executeUndoRedo(0);
                if(redosToAdd>0) {
                    topRedo=NULL;
                    redosToAdd = 0;
                }
                undosToAdd = undosToAdd +1;
                delete(start, end);
            }
            redosToExecute = 0;
            undosToExecute = 0;
            return 0;

        }
    }
}

int parseInput(int val){
    int performActionAfterParsing;
    currentCommand = val ? strtok(inputBuffer, "\n") : strtok(NULL, "\n");
    if(currentCommand == NULL || currentCommand[0] == QUIT_C)
        return 1;
    int numberOfCharacters = sscanf(currentCommand, "%d%c%d%c" , &start , &firstCharInInputCommand , &end , &secondCharInInputCommand);
    if(numberOfCharacters==2) {
        if ( undosToAdd > 0 && firstCharInInputCommand == UNDO_C ) {
            undosToExecute = undosToExecute + start;
            if (undosToExecute > undosToAdd)
                undosToExecute = undosToAdd;
            performActionAfterParsing = parseInputWithTracking();
            return performActionAfterParsing ? 1 : 2;
        }
        else if(redosToAdd>0 &&  firstCharInInputCommand == REDO_C)
        {
            redosToExecute = redosToExecute + start;
            if(redosToExecute>redosToAdd)
                redosToExecute = redosToAdd;
            performActionAfterParsing = parseInputWithTracking();
            return performActionAfterParsing ? 1 : 2;
        }
        else
            return 2;
    }
    else return numberOfCharacters == 1 ?  1 : 0;
}

/**********************************************************
                     INPUT HANDLERS
**********************************************************/

void storeInput()
{
    size_t size = 1000000000, totalNumberOfCharacters;
    totalNumberOfCharacters = fread(inputBuffer, 1, size , stdin);
    inputBuffer[totalNumberOfCharacters] = '\0';
}

void handleInput(){
    int performActionAfterParsing;
    performActionAfterParsing = parseInput(1);

    while (1) {
        if (performActionAfterParsing == 1)
            return;
        else if (performActionAfterParsing != 2) {
            if (secondCharInInputCommand == PRINT_C)
                print(start, end);
            else if(secondCharInInputCommand == CHANGE_C || secondCharInInputCommand == DELETE_C) {
                undosToAdd++;
                if (secondCharInInputCommand == CHANGE_C)
                    change(start, end);
                else if (secondCharInInputCommand == DELETE_C)
                    delete(start, end);
                if (redosToAdd > 0) {
                    topRedo = NULL;
                    redosToAdd = 0;
                }
            }
        }
        performActionAfterParsing = parseInput(0);
    }
}

/**********************************************************
                             MAIN
**********************************************************/

int main() {
    indexes = malloc(ALLOC_INCREMENT * sizeof(char *));
    inputBuffer = malloc( INPUT_SIZE );
    storeInput();
    handleInput();
    return 0;
}
