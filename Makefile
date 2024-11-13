# makefile com as seguintes regras: clean (remove todos os executáveis, .o e outros temporários), build (compila e gera o executável), e run (executa o comando criado) - o comportamento default deve ser build.


default: build

clean:
	/bin/rm -f ep1_francisco_aragao

build:
	gcc EP1_Francisco_Aragao.c -o ep1_francisco_aragao

run:
	./ep1_francisco_aragao
