#ifndef API_H_
#define API_H_

#include "Version.h"
#include "Bot.h"
#include "Client.h"
#include "State.h"
#include "Command.h"

// Have the plugins include json automatically so they can use config
#include "../../jsoncpp-0.8.0/include/json/json.h"
#pragma comment(lib, "lib_json.lib")

// Get rid of the ridiculous windows defines
#ifdef GetMessage
#undef GetMessage
#undef SendMessage
#endif

namespace api {

class Selector {
public:
    virtual std::shared_ptr<Player> Select(Bot* bot) = 0;
};

typedef std::shared_ptr<Selector> SelectorPtr;

} // ns

#endif
