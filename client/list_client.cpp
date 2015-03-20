#include "header.h"

/* Calculate Hash List Insert */

void ListC :: insert(DataC x){

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
		last = tmp;	//tmp->next = NULL
		nodes++;
	}
}

/* Calculate Hash List Out */

DataC ListC :: out(){

	DataC item;
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
		return NULL;
	}

	return item;

}

/* Calculate Hash Printing List */

void ListC :: print(){

	if( nodes == 0){
		cout<<"Empty List"<<endl;
		return;
	}
	Node* temp = first;
	cout<<"[ ";
	while( temp != NULL){
		cout<< temp->data<<" ]---[ ";
		temp = temp->next;
	}
	cout<<"NULL ]"<<endl;
}
/*-----------------------------------------------*/
/*  Hash Tree Sorted List Insert(By FileName) */

void ListHash :: insert(DataHT& tree, string FileName,int filesize){

        if(nodes==0){
                Node* tmp;
                tmp = new Node(tree, FileName,filesize);
                first = tmp;
                last = tmp;
                nodes++;
        }
        else{
                Node* tmp;
                tmp = new Node(tree,FileName,filesize);
		Node* moving = first;

		int counter = 0;
		/* Firstly i will check if the the element go first */
		if( FileName <= first -> FileName ){
			Node* hold = first;
			first = tmp;
			first->next = hold;
			nodes++;

			return;
		}
		/* Now check if is inside */
		Node* moving_infront = moving->next;
		while( moving_infront != NULL){
			if( FileName < moving_infront -> FileName ){
				moving->next = tmp;
				tmp->next = moving_infront;
				nodes++;
				return;
			}
			moving_infront = moving_infront->next;
			moving = moving->next;
		}
		/* And Now if the element must be entered at last */
		last->next = tmp;
		last = tmp;
		nodes++;
        }
}


/* Printing Hash Tree List */

void ListHash :: print(int height, int fan_out){

        if( nodes == 0){
                cout<<"Empty List"<<endl;
                return;
        }
        Node* temp = first;
        cout<<"[ ";
        while( temp != NULL){
		cout<<"***************************************************"<<endl;
                cout<< temp->FileName<<endl;
		int h=height-1;
		while(h>=0){
			for(int i=0; i<power(fan_out,h);i++){
        			MD5Print( temp->data[ h ][ i ].digest );
        			cout<<endl;
			}
		cout<<"---------------------------------"<<endl;
		h--;
		}


                temp = temp->next;
        }
	cout<<"***************************************************"<<endl;

}

string ListHash :: name(int place){

	if( nodes == 0 )
		return "EMPTY LIST";
	int counter = 0;
	Node* temp;
	temp = first;
	while( temp != NULL){
		if(counter == place)
			return temp->FileName;
		temp = temp->next;
		counter++;
	}
}
int ListHash :: size(int place){

        if( nodes == 0 )
                return -1;
        int counter = 0;
        Node* temp;
        temp = first;
        while( temp != NULL){
                if(counter == place)
                        return temp->filesize;
                temp = temp->next;
                counter++;
        }
}
void condition_w8(){sleep(1);}
DataHT& ListHash :: tree(int place){

        if( nodes == 0 )
                cout<<"tree compare error!"<<endl;
        int counter = 0;
        Node* temp;
        temp = first;

	while( temp != NULL){
                if(counter == place)
                        return temp->data;
                temp = temp->next;
                counter++;
	}
}
