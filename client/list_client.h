typedef string DataC;
typedef struct Encoded** DataHT;



class ListC
{

        struct Node
	{
                DataC    data;
                Node*   next;

                Node (DataC data_, Node* next_ = NULL) : data(data_), next(next_) {}
        };


        Node*           first;
        Node*           last;
        unsigned int    nodes;
public:
	ListC():nodes(0),first(NULL),last(first){}
	~ListC ()
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
	void insert(DataC);
	void print();
	DataC out();	//out and return the first element

};

/* Sorted list with hash trees */
class ListHash
{

        struct Node
        {	string FileName;
                DataHT    data;
		int filesize;
                Node*   next;

                Node (DataHT data_, string FileName_ ,int filesize_ = -1, Node* next_ = NULL) :
			data(data_),FileName(FileName_),filesize(filesize_) ,next(next_) {}
        };


        Node*           first;
        Node*           last;
        unsigned int    nodes;
public:
        ListHash():nodes(0),first(NULL),last(first){}
        ~ListHash ()
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
        void insert(DataHT&,string,int);
        void print(int,int);
	string name(int place);
	int size(int place);
	DataHT& tree(int place);
        DataHT out();    //out and return the first element

};

