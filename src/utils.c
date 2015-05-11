#include "includes.h"

char *input_string(char *str, size_t size, FILE *file)
{
    str = fgets(str, size, file);
    if(str[strlen(str)-1] == '\n')
        str[strlen(str)-1] = '\0';

    return str;
}

/**
 * Renvoie 1 si le fichier existe, 0 sinon.
 */
unsigned short file_exists(const char *path)
{
    FILE *file;

    if((file = fopen(path, "rb")))
    {
        fclose(file);
        return 1;
    }
    else
       return 0;
}

/**
 * Renvoie 1 si le dossier est créé, 0 sinon.
 */
unsigned short create_directory(const char *path)
{
    #ifdef WIN32
        if(CreateDirectory(path, NULL) == 0)
            return 1;
        else
            return 0;
    #elif defined (linux)
        if(mkdir(path, (mode_t)0770) == 0)
            return 1;
        else
            return 0;
    #else
        return 0;
    #endif
}

/**
 * Création d'un nouveau fichier XML
 */
unsigned short createXmlFile(const char *path, const char *root)
{
    FILE *file;
    char tag[strlen(root)+4];

    create_directory(DATA_FOLDER);

    if((file = fopen(path, "wb")))
    {
        sprintf(tag, "<%s/>", root);
        fprintf(file, "%s\n%s", XML_HEADER, tag);

        fclose(file);
        return 1;
    }
    else
        return 0;
}

/**
 * Revoie un tableau de chaînes en fonction du délimiteur spécifié.
 * Le tableau renvoyé doit être libéré après utilisation.
 */
char **explode(char *src, const char *delim, unsigned short *size)
{
    char **array = NULL; //Tableau de retour
    unsigned short sizeArr = 0; //Taille du tableau de retour

    char *part = NULL; //Portion de chaîne extraite grâce au délimiteur
    unsigned short sizePart; //Taille de cette sous-chaîne

    char *str = src; //Copie de la chaîne d'origine pour traitement
    unsigned short sizeDelim = strlen(delim); //Taille du délimiteur, pour décaler le pointeur après chaque extraction

    //Tant qu'on trouve le délimiteur dans la chaîne de traitement...
    while((part = strstr(str, delim)) != NULL)
    {
        sizePart = part-str;

        //Si la chaine trouvé n'est pas vide
        if(sizePart != 0)
        {
            //On alloue une case supplémentaire à notre tableau de chaînes
            sizeArr++;
            array = realloc(array, sizeof(char*) * sizeArr);

            //On alloue la chaine du tableau
            array[sizeArr-1] = malloc(sizeof(char) * (sizePart+1));
            strncpy(array[sizeArr-1], str, sizePart);
            array[sizeArr-1][sizePart] = '\0';
        }

        //On décale le pointeur str pour reprendre la boucle après le délimiteur
        part = part + sizeDelim;
        str = part;
    }

    //Si on ne trouve plus de délimiteur mais que la chaîne n'est pas vide, on récupère ce qu'il reste
    if(strlen(str) != 0)
    {
        unsigned short lastcr;

        sizePart = strlen(str);
        //Vaut 1 si le dernier caractère de la chaîne est un retour chariot
        lastcr = (str[sizePart-1] == '\n');

        sizeArr++;
        array = realloc(array,sizeof(char*) * sizeArr);

        //Si le dernier caractère de la chaîne est un \n, +0 car il sera écrasé par un \0 ; sinon, +1 car on ajoutera un \0 à la fin
        array[sizeArr-1]= malloc(sizeof(char) * (sizePart + !lastcr));
        strncpy(array[sizeArr-1],str,sizePart);

        //Si le dernier caractère de la chaîne est un \n, on le remplace par un \0, sinon on ajoute le \0 à la fin de la chaîne
        array[sizeArr-1][sizePart - lastcr] = '\0';
    }

    //On met à jour le pointeur size avec le nombre de chaînes contenues dans le tableau
    *size = sizeArr;

    return array;
}


unsigned short regex_verification(const char *str, unsigned short mode)
{
    regex_t preg;
    char regex[1024];

    switch(mode)
    {
        case IPPORT_ADDR :
            strcpy(regex, "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):[0-9]{1,5}$");
            break;

        case IP_ADDR :
            strcpy(regex, "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                          "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
            break;

        case MAC_ADDR :
            strcpy(regex,"^([0-9A-F]{2}[:-]){5}([0-9A-F]{2})$");
            break;

        default :
            return 0;

    }

    if(regcomp(&preg, regex, REG_EXTENDED) == 0)
    {
        int match;

        match = regexec(&preg, str, 0, NULL, 0);

        regfree(&preg);

        if(match == 0)
            return 1;
        else
            return 0;
    }

    return 0;
}

unsigned short *char_to_bin(const char *str, unsigned int *size)
{
    unsigned short *binary;
    int i, j, bit, size_tab = 1;
    unsigned int len;


    binary = malloc(sizeof(unsigned short));
    len = *size;
    *size = 0;

    //Convertit en bit le message caractère par caractère
    for(i = 0; i < len; i++)
    {
        for(j = 7; j >= 0; j--)
        {
            bit = (str[i] & (1 << j)) ? 1 : 0;

            if(*size >= size_tab)
            {
                size_tab *= 2;
                binary = realloc(binary, size_tab * sizeof(unsigned short));
            }

            binary[*size] = bit;

            (*size)++;
        }
    }

    return binary;
}

char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length)
{
    int mod_table[] = {0, 2, 1}, i, j;

    char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                            'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                            'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                            'w', 'x', 'y', 'z', '0', '1', '2', '3',
                            '4', '5', '6', '7', '8', '9', '+', '/'};

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(((*output_length)+2) *sizeof(char));
    if (encoded_data == NULL) return NULL;

    for (i = 0, j = 0; i < input_length;)
    {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';
    encoded_data[*output_length] = '|';
    encoded_data[*output_length+1] = '\0';

    return encoded_data;
}


char *base64_decode(const unsigned char *data, size_t input_length, size_t *output_length)
{
    int i, j;
    char *decoding_table = build_decoding_table();

    if (decoding_table == NULL) return NULL;

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    char *decoded_data = malloc(((*output_length)+1) * sizeof(char));
    if (decoded_data == NULL) return NULL;

    for (i = 0, j = 0; i < input_length;)
    {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
                        + (sextet_b << 2 * 6)
                        + (sextet_c << 1 * 6)
                        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    free(decoding_table);
    decoded_data[*output_length] = '\0';
    return decoded_data;
}


char *build_decoding_table()
{
    int i;
    char *decoding_table = NULL;

    char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                            'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                            'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                            'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                            'w', 'x', 'y', 'z', '0', '1', '2', '3',
                            '4', '5', '6', '7', '8', '9', '+', '/'};

    decoding_table = malloc(256);

    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;

    return decoding_table;
}

time_t getWeekStart(time_t *timestamp)
{
    time_t start = *timestamp;
    struct tm time = *localtime(timestamp);
    short day = time.tm_wday-1;

    if(day == -1)
        day = 6;

    start -= time.tm_sec + time.tm_min*60 + time.tm_hour*3600 + day*TIMESTAMP_DAY;
    return start;
}

time_t getWeekEnd(time_t *timestamp)
{
    time_t end = *timestamp;
    struct tm time = *localtime(timestamp);
    short day = time.tm_wday-1;

    if(day == -1)
        day = 6;

    end -= time.tm_sec + time.tm_min*60 + time.tm_hour*3600;
    end += (7-day)*TIMESTAMP_DAY;
    return end;
}
