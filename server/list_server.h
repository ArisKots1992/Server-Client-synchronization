struct msg_id {
        string message_c;
        int client_id;
};

typedef struct msg_id DataS;


class ListS
{

        struct Node
        {
                DataS    data;
                Node*   next;

                Node (DataS data_, Node* next_ = NULL) : data(data_), next(next_) {}
        };


        Node*           first;
        Node*           last;
        unsigned int    nodes;
public:
        ListS():nodes(0),first(NULL),last(first){}
        ~ListS ()
        {
                Node *temp, *i = first;
                while (i != NULL)
                {
                        temp = i;
                        i = i->next;
                        delete temp;
                }
        }
        int size(){
                return nodes;
        }
        void insert(DataS);
        void print();
        DataS out();    //out and return the first element

};


