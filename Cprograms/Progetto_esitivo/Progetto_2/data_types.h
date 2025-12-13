#ifndef DATATYPES_H
#define DATATYPES_H

#include<stdbool.h>
#include<time.h>
#include "config1.h"

typedef enum{
    IDLE,
    EN_ROUTE_TO_SCENE,
    ON_SCENE,
    RETURNING_TO_BASE
}rescuer_status_t;


typedef struct{
    char * rescuer_type_name; //allocato dinamicamente
    int speed;
    int x;
    int y;
}rescuer_type_t;

typedef struct{
    int id;
    int x;
    int y;
    rescuer_type_t* rescuer;
    rescuer_status_t status;
}rescuer_digital_twin_t;



typedef struct{
    rescuer_type_t* type;
    int required_count;
    int time_to_manage; 
}rescuer_request_t;


typedef struct{
    short priority;
    char* emergency_desc;   //da allocare dinamicamente
    rescuer_request_t* rescuers;    //array dei soccorritori
    int rescuer_required_number;
}emergency_type_t;


typedef enum{
    WAITING,
    ASSIGNED,
    IN_PROGRESS,
    PAUSED,
    COMPLETED,
    CANCELED,
    TIMEOUT
}emergency_status_t;

typedef struct{
    char emergency_name[EMERGENCY_NAME_LENGTH];
    int x;
    int y;
    time_t timestamp;
}emergency_request_t;

typedef struct{
    emergency_type_t* type; //metto puntatore finche prof non risponde
    emergency_status_t status;
    int x;
    int y;
    time_t time;
    int resquer_cont;
    rescuer_digital_twin_t ** rescuer_dt;
}emergency_t;


//strutture di supporto non definite da prof

typedef struct{
    char queue_name[LINE_LENGTH];
    int grid_height;
    int grid_width;
}environment_t;

//questa struttura mi serve di supporto visto che non possso apparentemente modificare
//strutture dati date

typedef struct{
    environment_t config_env;
    rescuer_type_t* rescuers_type_array;
    int rescuer_type_num;                       
    emergency_type_t *emergency_types_array;    
    int emergency_type_num;
    int* instances_per_rescuer_type; // Array parallelo a rescuers_type_array, contiene <num> per ogni tipo                   
    int total_digital_twin_da_creare; 
}system_config_t;




#endif 