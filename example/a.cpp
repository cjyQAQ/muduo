#include <iostream>
using namespace std;

class a
{
private:
    int b;
public:
    a();
    ~a();
};

a::a():b(10)
{

}

a::~a()
{
}

void func(a *aa)
{
    if(aa == nullptr)
    {
        cout<<"is null"<<endl;
    }
    else
    {
        cout<<"is not null"<<endl;
    }
}

int main()
{
    a a1;
    func(&a1);
    return 0;
}
