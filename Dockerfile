FROM	gcc:13 AS entr_build
COPY	suid_entrypoint.c suid_entrypoint.c
RUN	gcc -static -Wall -o suid_entrypoint suid_entrypoint.c && \
	chmod u+s suid_entrypoint
ENTRYPOINT [ "/suid_entrypoint" ]
CMD	["bash"]