#include <vector>
#include <tuple>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <initializer_list>

namespace Utils{
    template <class Key, class Value>
    class Bimap {
        private:
            std::vector<Key> keys;
            std::vector<Value> values;

        public:
            Bimap(){}
            Bimap(const std::initializer_list<std::pair<Key, Value>>& mapValues){
                for(const std::pair<Key, Value> item: mapValues){
                    keys.push_back(item.first);
                    values.push_back(item.second);
                }
            }

            void Add(const Key& key, const Value& value){
                key.push_back(key);
                value.push_back(value);
            }

            Value operator[](const Key& key){
                typename std::vector<Key>::iterator pos = std::find(keys.begin(), keys.end(), key);

                if(pos == keys.end()){
                    std::stringstream notFound;
                    notFound << key;
        
                    throw std::out_of_range("Key not found: '" + notFound.str() + "'");
                }

                return values[pos - keys.begin()];
            }

            Key operator[](const Value& value){
                typename std::vector<Value>::iterator pos = std::find(values.begin(), values.end(), value);

                if(pos == values.end()){
                    std::stringstream notFound;
                    notFound << value;
        
                    throw std::out_of_range("Value not found: '" + notFound.str() + "'");
                }

                return keys[pos - values.begin()];
            }
    };
};
