FROM	gcc:13 AS entr_build
COPY	suid_entrypoint.c suid_entrypoint.c
RUN	gcc -static -o suid_entrypoint suid_entrypoint.c && \
	chmod u+s suid_entrypoint
ENTRYPOINT [ "/suid_entrypoint" ]