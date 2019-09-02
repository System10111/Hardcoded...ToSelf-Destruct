#include <vector>
#include <map>

struct JsonObj
{
    std::map<std::string_view,JsonObj> objs;
}