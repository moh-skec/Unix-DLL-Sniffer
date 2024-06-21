all : dllsniffer dllsniffer2 dllsniffer_pro
	@echo "ready ..."
dllsniffer : dllsniffer.o
	@gcc dllsniffer.o -o dllsniffer
dllsniffer2 : dllsniffer2.o
	@gcc dllsniffer2.o -o dllsniffer2
dllsniffer_pro : dllsniffer_pro.o
	@gcc dllsniffer_pro.o -o dllsniffer_pro

dllsniffer.o : dllsniffer.c
	@gcc -c dllsniffer.c
dllsniffer2.o : dllsniffer2.c
	@gcc -c dllsniffer2.c
dllsniffer_pro.o : dllsniffer_pro.c
	@gcc -c dllsniffer_pro.c

clean :
	@echo "cleaning ..."
	@rm dllsniffer*.o dllsniffer dllsniffer2 dllsniffer_pro 