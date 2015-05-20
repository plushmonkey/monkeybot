import cleverbot, sys

if len(sys.argv) <= 1:
    sys.exit(0)

try:
    bot = cleverbot.Cleverbot()
    print bot.ask(sys.argv[1])
except:
    print ''