#include "header.h"

/* Inserting Server Message */
void ListS :: insert(DataS x){

        if(nodes==0){
                Node* tmp;
                tmp = new Node(x);
                first = tmp;
                last = tmp;
                nodes++;
        }
        else{
                Node* tmp;
                tmp = new Node(x);
                last->next = tmp;
                last = tmp;     //tmp->next = NULL
                nodes++;
        }
}


/* Server Message Printing List */

void ListS :: print(){

        if( nodes == 0){
                cout<<"Empty List"<<endl;
                return;
        }
        Node* temp = first;
        cout<<"[ ";
        while( temp != NULL){
                cout<< temp->data.message_c<<temp->data.client_id<<" ]---[ ";
                temp = temp->next;
        }
        cout<<"NULL ]"<<endl;
}

/* Server Queue Out */
DataS ListS :: out(){

        DataS item;
        if( nodes > 1){
                Node* temp;
                temp = first;
                item = temp->data;
                first = first->next;
                nodes--;
                delete temp;
        }
        else if( nodes == 1){
                Node* temp;
                temp = first;
                item = temp->data;
                first = first->next;
                last = first;
                nodes--;
                delete temp;
        }
        else{
        /* already empty */
                //return ;
        }

        return item;

}


