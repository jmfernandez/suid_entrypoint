# SUID entrypoint to be used for docker images

This repo contains both the source code of the entrypoint and a Dockerfile for the proof of concept.

This entrypoint elevates its privileges to dynamically create both a `dynuser` user
and a `dyngroup` group, which match the uid and gid passed to docker run. Additionally, if
docker socket is available at `/run/docker.sock`, the entrypoint dynamically creates a
`dyndocker` group and adds `dynuser` to `dyndocker`.

## Building the proof of concept

```bash
docker build -t suid_entrypoint:latest .
```

## Testing the proof of concept

```bash
docker run --rm -ti   -u $(id -u):$(id -g)     --device=/dev/fuse   -v /run/docker.sock:/run/docker.sock suid_entrypoint:latest /bin/bash
```

## Limitations

SUID binaries do not work when `--security-opt no-new-privileges` is passed to docker instance invocation:

```bash
docker run --rm -ti   -u $(id -u):$(id -g)   --device=/dev/fuse   --security-opt no-new-privileges -v /run/docker.sock:/run/docker.sock suid_entrypoint:latest /bin/bash
```

```
I have no name!@56e8470c59c2:/$ 
```
