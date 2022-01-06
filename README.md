# Security

This repo contains my "hands on" in the Security domain.

Flush_Reload Attack:

I developed this code for flush and reload attack on a shared library<redacted>
Flush and Reload is a popular Side Channel Attack where the attacker  tries to access the memory locations 
accessed by the victim by flushing & reloading the memory addresses/address. If the victim accesses the address after a flush, 
its trace stays in the cache and reload happens quickly.
