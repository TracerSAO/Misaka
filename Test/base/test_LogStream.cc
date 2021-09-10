#include "../../base/LogStream.h"
#include "../../base/StringPiece.h"

#include <iostream>
using std::cout;
using std::endl;

using namespace Misaka;
using namespace detail;


int main()
{
    LogStream os;
    os << "bilibili~ 2233" << " || " << 2233;
    cout << os.buffer().data() << endl;
}