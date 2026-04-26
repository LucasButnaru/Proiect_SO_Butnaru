# AI Usage Documentation

Tool Used: Gemini

Prompts Given:

1. Am o structură "Report" în C și am nevoie de o funcție "int parse_condition(const char *input, char *field, char *op, char *value);" care să ia un string de formatul "field:operator:value" și să îl spargă în cele 3 componente.
2. Structura mea "Report" are câmpurile "severity" (int), "category" (string), "inspector" (string) și "timestamp" (time_t). Creează funcția "int match_condition(Report *r, const char *field, const char *op, const char *value);" care evaluează condiția matematică sau logică și returnează 1 dacă e adevărat, altfel 0.

What was generated:

AI-ul a generat cele două funcții. Pentru parse_condition a folosit funcția strchr pentru a căuta manual caracterul ":" de două ori și strncpy/strcpy pentru a extrage substring-urile. Pentru match_condition a folosit un set de if/else if care fac strcmp pentru a stabili câmpul vizat, urmat de o conversie a parametrului value (cu atoi sau atol) și evaluarea operatorului matematic/logic.

What I changed and why:

Am integrat codul în arhitectura generală a programului meu. A trebuit să aloc manual memorie statică (char field[32], etc.) în bucla mea while din logica principală de filtrare înainte de a pasa pointerii către funcția generată de AI, deoarece funcția doar asignează valori și se așteaptă la memorie pre-alocată.

What I learned:

Am învățat cum se face parsarea eficientă a șirurilor de caractere în C folosind pointeri de memorie și aritmetica pointerilor (ex: calcularea lungimii cu "colon1 - input"). De asemenea, am înțeles cum pot decupla logica de parsare a inputului de logica de validare propriu-zisă pentru a avea cod modular.