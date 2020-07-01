#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define STD_LENGHT 100

typedef struct input{               //elemento della lista di possibili input leggibili in corrispondenza di uno stato N
    char to_read[2];                //carattere da leggere
    struct action* actions;         //puntatore alla lista delle possibili azioni da eseguire
    int possible_actions;           //numero delle possibili azioni da eseguire
    struct input* next_char;        //puntatore al prossimo input leggibile
    struct action* last_action;     //puntatore all'ultima azione inserita
} Input;
typedef struct action{              //elemento del vettore delle possibili azioni eseguibili in corrispondenza di uno stato N e un carattere di input C
    char to_write[2];
    char movement[2];
    int next_state;
    struct action* next_action;
} Action;
typedef struct node{                //nodo dell'albero della computazione (modifica la stessa stringa se c'è solo un'azione possibile)
    char* string_head;              //puntatore al carattere iniziale della stringa
    int character;                  //posizione del carattere da analizzare
    int actual_state;               //stato attuale
    int actions_done;               //passi già fatti
    struct node* next;              //puntatore al padre
} Queue;
typedef struct state{               //elemento dell'array di stati (puntatori ai possibili input leggibili)
    Input* readable_char;
    bool accepting;
} State;

State* states=NULL;
char* read=NULL;
char* string=NULL;
int numberOfStates=0;
int maxSteps=0;
char output;
int ret;
Queue* head=NULL;
Queue* tail=NULL;

void readTransitionFunction();
void readAcceptableStates();
void saveState(int readValue);
Input* saveInputChar(int state);
void addNewAction(int state, Input* added_char, char* char_to_write, char* head_movement, int next_state);
void readString();
void startSimulation();
bool accepted(Queue* actual);
void doAction(Action* step, Queue* actual_node, char* analyzing);
bool nonDetPath(Queue* actual, Action* to_do);
void duplicateValues(Queue* source, Queue* destination);
void freeQueue();
void freeTransitionFunction();
void freeCharList(Input* head);
void freeActionList(Action* first);
void charNotFound();
void detPath(Queue* actual, Action* to_do);
bool looping(Queue* actual, Action* to_do);

int main(){

    read = malloc(STD_LENGHT*sizeof(char));
    ret = fscanf(stdin, "%s", read);            //legge tr
    readTransitionFunction();                   //legge tutta la funzione di transizione e salva i valori; termina con read = acc
    ret = fscanf(stdin, "%s", read);            //legge il primo stato di accettazione
    readAcceptableStates();                     //legge tutti gli stati di accettazione e li salva nella lista *acc; termina con read = max
    ret = fscanf(stdin, "%s", read);            //legge il valore successivo a max
    maxSteps = strtol(read, NULL, 10);          //lo converte e lo salva in maxSteps
    ret = fscanf(stdin, "%s", read);            //leggo run
    ret = fscanf(stdin, "%c", read);            //leggo il \n
    free(read);                                 //libero la memoria di read
    read = NULL;

    read = calloc(2, sizeof(char));                 //rialloco memoria per la stringa
    ret = fscanf(stdin, "%c", read);             //leggo la stringa
    while(!feof(stdin)){
        readString();
        output = '0';
        startSimulation();                      //effettuo la simulazione
        fprintf(stdout, "%c\n", output);        //stampo l'output
        string = NULL;
        ret = fscanf(stdin, "%c", read);              //leggo il carattere successivo
    }

    free(read);
    freeTransitionFunction();

    return 0;
}


//------------------------------------------------------------FUNZIONI PER LA LETTURA DELLA FUNZIONE DI TRANSIZIONE-----------------------------------------------------------------------------

void readTransitionFunction(){
    int tmp, next_state;
    Input* added_char;
    char* to_write = NULL;
    char* movement = NULL;

    to_write = calloc(2, sizeof(char));
    movement = calloc(2, sizeof(char));
    ret = fscanf(stdin, "%s", read);                //legge il primo stato
    while(strcmp(read, "acc") != 0){
        tmp = strtol(read, NULL, 10);               //converte lo stato letto ad un intero
        if(tmp >= numberOfStates)                   //se lo stato letto è maggiore o uguale a quelli già esistenti
            saveState(tmp);                         //ridimensiono il vettore di stati
        ret = fscanf(stdin, "%s", read);          //legge il carattere di input
        added_char = saveInputChar(tmp);    //salva il carattere nella lista dei possibili caratteri leggibili e ritorna la posizione in cui è stato inserito il carattere

        ret = fscanf(stdin, "%s", to_write);      //legge il carattere di output
        ret = fscanf(stdin, "%s", movement);      //legge il movimento della testina
        ret = fscanf(stdin, "%d", &next_state);   //legge lo stato successivo
        addNewAction(tmp, added_char, to_write, movement, next_state);

        ret = fscanf(stdin, "%s", read);          //legge il carattere successivo (verrà poi verifificato dal while se è acc o meno)
    }

    free(to_write);
    free(movement);
}

void saveState(int readValue){
    if(states == NULL){                                             //prima chiamata -> alloco memoria per il vettore di stati
        numberOfStates = readValue + 1;
        states = calloc(numberOfStates, sizeof(State));
    }
    else{                                                           //caso in cui bisogna aggiungere 1 o + stati
        int tmp;
        tmp = numberOfStates;
        numberOfStates = readValue + 1;
        states = realloc(states, numberOfStates*sizeof(State));
        for(int i=tmp; i<numberOfStates; i++){
            states[i].readable_char = NULL;
            states[i].accepting = false;
        }
    }
}

Input* saveInputChar(int state){       //aggiunge il carattere se non c'è e ritorna il puntatore alla struttura contenente il carattere (il numero delle azioni possibili viene aggiornato)
    Input* new_char=NULL;
    Input* support = NULL;

    if(states[state].readable_char == NULL){            //LO STATO "STATE" NON CONTIENE CARATTERI DI INPUT----------------------------------------------------------------------------------------------
        new_char = calloc(1, sizeof(Input));            //creo la struttura
        strcpy(new_char->to_read, read);                //ci copio dentro il carattere
        new_char->possible_actions = 1;                 //assegno l'azione possibile
        states[state].readable_char = new_char;         //la assegno al primo elemento della lista dei readable char
        return new_char;                                //ritorno il puntatore all'elemento appena inserito
    }
    else{                                               //LO STATO "STATE" CONTIENE GIà CARATTERI DI INPUT----------------------------------------------------------------------
        support = states[state].readable_char;
        while(support != NULL){                     //scorro tutti i possibili caratteri di input
            if(strcmp(support->to_read, read) == 0){     //IL CARATTERE HA GIà UN AZIONE CORRISPONDENTE-----------------------------
                support->possible_actions++;        //incremento il numero dele sue azioni possibili
                return support;                     //e ritorno il puntatore all'elemento
            }
            if(support->next_char == NULL)          //se sono all'ultimo elemento della lista
                break;                              //esco dal ciclo tenendomi così la posizione dell'ultimo elemento
            else                                    //altrimenti
                support = support->next_char;       //passo all'elemento successivo
        }                                           //IL CARATTERE NON HA AZIONI CORRISPONDENTI---------------------------------
        new_char = calloc(1, sizeof(Input));        //creo il nuovo carattere
        strcpy(new_char->to_read, read);            //lo copio
        new_char->possible_actions = 1;             //assegno l'azione possibile
        support->next_char = new_char;              //faccio puntare l'ultimo elemento della lista al nuovo carattere
        return new_char;                            //ritorno la sua posizione
    }
}

void addNewAction(int state, Input* added_char, char* char_to_write, char* head_movement, int next_state){       //crea l'azione e ci inserisce i valori
    Action* new_action;

    new_action = calloc(1, sizeof(Action));         //creo l'azione
    strcpy(new_action->to_write, char_to_write);    //e ci inserisco i valori
    strcpy(new_action->movement, head_movement);
    new_action->next_state = next_state;
    if(next_state >= numberOfStates)
        saveState(next_state);

    if(added_char->possible_actions == 1){          //NEL CASO IN CUI CI SIA UN'UNICA AZIONE POSSIBILE----------------------------------------------------------------------
        added_char->actions = new_action;           //faccio puntare l'inizio della lista all'elemento appena creato
        added_char->last_action = new_action;       //e faccio puntare anche l'ultima azione
    }
    else{                                                       //NEL CASO DI PIù AZIONI------------------------------------------------------------------------------------
        added_char->last_action->next_action = new_action;      //aggiungo in fondo alla lista delle possibili azioni il nuovo elemento
        added_char->last_action = new_action;                   //agiorno il puntatore all'ultima azione
    }
}


//------------------------------------------------------------FUNZIONI PER LA LETTURA DEGLI STATI DI ACCETTAZIONE-------------------------------------------------------------------------------

void readAcceptableStates(){
    int state;

    while(strcmp(read, "max") != 0){
        state = strtol(read, NULL, 10);                 //converto la stringa in un intero
        if(state >= numberOfStates)
            saveState(state);
        states[state].accepting = true;
        ret = fscanf(stdin, "%s", read);                      //leggo infine il prossimo valore
    }
}


//-----------------------------------------------------------------FUNZIONI PER LA LETTURA DELLE STRINGHE---------------------------------------------------------------------------------------

void readString(){
    while(strcmp(read, "\n") != 0 && !feof(stdin)){     //finchè non è finita la riga o l'input
        if(string == NULL){                             //SE LA STRINGA è VUOTA
            string = calloc(2, sizeof(char));           //creo l'elemento contenente il carattere
            strcpy(string, read);                       //lo copio
        }
        else{                                           //SE LA STRINGA NON è VUOTA
            string = realloc(string, (strlen(string)+2)*sizeof(char));      //rialloco la memoria
            strcat(string, read);                                           //ci copio il carattere
        }
        ret = fscanf(stdin, "%c", read);                      //leggo il carattere successivo
    }
}

//-----------------------------------------------------------FUNZIONI PER LA SIMULAZIONE DELLA MACCHINA DI TURING-------------------------------------------------------------------------------

void startSimulation(){
    Input* char_structure=NULL;
    char* to_analyze=NULL;
    State* actual_state;
    Action* send;

    head = calloc(1, sizeof(Queue));
    head->string_head = string;
    tail = head;
    actual_state = &states[head->actual_state];

    while(!actual_state->accepting){
        char_structure = actual_state->readable_char;
        to_analyze = &head->string_head[head->character];
        while(char_structure != NULL){                                              //scorro l'array dei readable_char dello stato attuale
            if(*char_structure->to_read == *to_analyze){                            //se trovo un carattere uguale
                break;                                                              //mi fermo
            }
            char_structure = char_structure->next_char;
        }

        if(char_structure != NULL){
            if(char_structure->possible_actions == 1){                                      //PERCORSO DETERMINISTICO
                send = char_structure->actions;
                if(!states[send->next_state].accepting){
                    detPath(head, send);
                }
                else
                    break;
            }
            else{                                                                           //PERCORSO NON DETERMINISTICO
                send = char_structure->actions;
                if(!nonDetPath(head, send)){
                    break;
                }
            }
        }
        else{
            charNotFound();
        }

        if(head == NULL)
            return;
        else
            actual_state = &states[head->actual_state];
    }
    freeQueue();                 //libera tutto l'albero delle computazioni
    head = NULL;
    tail = NULL;
    output = '1';            //setto l'output a 1 (arrivo a questo punto solo se ho trovato uno stato di accettazione)
}

void doAction(Action* step, Queue* actual_node, char* analyzing){             //effettua le modifiche sulla stringa di actual_node in base all'azione step
    *analyzing = *step->to_write;              //modifico il carattere
    actual_node->actual_state = step->next_state;   //cambio stato
    actual_node->actions_done++;                    //incremento il numero di azioni effettuate

    if(*step->movement == 'R'){                             //-------------------------------------MOVIMENTO TESTINA A DESTRA
        actual_node->character++;                           //sposto a destra la posizione del carattere
        if(actual_node->character == strlen(actual_node->string_head)){          //se il carattere è null..
            actual_node->string_head = realloc(actual_node->string_head, (strlen(actual_node->string_head)+2)*sizeof(char));    //rialloco la memoria
            strcat(actual_node->string_head, "_");                                                                              //ci aggiungo il carattere '_'
        }
    }
    else if(*step->movement == 'L'){                        //-------------------------------------MOVIMENTO TESTINA A SINISTRA
        if(actual_node->character == 0){                                            //se sono al primo carattere
            char* tmp = calloc(strlen(actual_node->string_head)+2, sizeof(char));   //rialloco la stringa
            strcpy(tmp, "_");                                                       //ci metto il carattere '_'
            strcat(tmp, actual_node->string_head);                                  //ci concateno la precedente
            free(actual_node->string_head);
            actual_node->string_head = tmp;
            tmp=NULL;
        }
        else                                //altrimenti
            actual_node->character--;       //decremento l'indice del carattere da analizzare
    }
}

void detPath(Queue* actual, Action* to_do){
    if(!looping(actual, to_do)){
        char* to_analyze = NULL;
        to_analyze = &head->string_head[head->character];
        doAction(to_do, head, to_analyze);
        if(head->actions_done == maxSteps){
            output = 'U';
            Queue* tmp = head->next;
            free(head->string_head);
            free(head);
            head = tmp;
        }
    }
    else{
        output = 'U';
        Queue* tmp = head->next;
        free(head->string_head);
        free(head);
        head = tmp;
    }
}

bool nonDetPath(Queue* actual, Action* to_do){  //aggiunge al nodo father un figlio e fa la prima azione. Se ce ne n'è un'altra salva il puntatore in father->next_action
    Queue* new=NULL;
    char* to_analyze=NULL;

    while(to_do != NULL){
        if(!states[to_do->next_state].accepting){
            if(((actual->actions_done+1) < maxSteps) && (!looping(actual, to_do))){
                new = calloc(1, sizeof(Queue));
                duplicateValues(actual, new);
                to_analyze = &new->string_head[new->character];
                doAction(to_do, new, to_analyze);
                tail->next = new;
                tail = new;
                new = NULL;
            }
            else{
                output = 'U';                            //metto l'output a U
            }
            to_do = to_do->next_action;
        }
        else{
            return false;
        }
    }
    new = head->next;
    free(head->string_head);
    free(head);
    head=new;
    return true;
}

void duplicateValues(Queue* source, Queue* destination){              //copia tutti i valori di source in destination
    destination->actual_state = source->actual_state;
    destination->actions_done = source->actions_done;
    destination->string_head = calloc(strlen(source->string_head)+1, sizeof(char));
    strcpy(destination->string_head, source->string_head);
    destination->character = source->character;
}

void charNotFound(){
    Queue* tmp = head->next;
    free(head->string_head);
    free(head);
    head = tmp;
}

bool looping(Queue* actual, Action* to_do){
    if((actual->string_head[actual->character] == *to_do->to_write) && (actual->actual_state == to_do->next_state) && (*to_do->movement == 'S'))
        return true;
    if((actual->character == 0) && (actual->string_head[actual->character] == '_') && (*to_do->movement == 'L') && (actual->actual_state == to_do->next_state))
        return true;
    return false;
}


//------------------------------------------------------------FUNZIONI PER IL DEALLOCAMENTO DELLE STRUTTURE DATI---------------------------------------------------------------------------------

void freeQueue(){          //libera ricorsivamente tutto l'albero (radice compresa)
    Queue* tmp=head;

    while(tmp!=NULL){
        tmp = head->next;
        free(head->string_head);
        free(head);
        head = tmp;
    }
}

void freeTransitionFunction(){          //libera il vettore di stati
    Input* to_free;
    for(int i=0; i<numberOfStates; i++){
        if(states[i].readable_char != NULL){
            to_free = states[i].readable_char;
            freeCharList(to_free);
        }
    }
    free(states);
}

void freeCharList(Input* head){         //libera la lista dei caratteri leggibili in un determinato stato
    Input* next = head->next_char;
    while(next != NULL){
        freeActionList(head->actions);
        free(head);
        head = next;
        next = head->next_char;
    }
    freeActionList(head->actions);
    free(head);
}

void freeActionList(Action* first){     //libera la lista delle azioni eseguibili in corrispondenza di un carattere
    Action* next = first->next_action;
    while(next != NULL){
        free(first);
        first = next;
        next = first->next_action;
    }
    free(first);
}
