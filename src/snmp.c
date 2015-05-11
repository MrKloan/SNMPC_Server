#include "includes.h"

SNMPRequest *InitRequest(Device *device, COMMUNITIES cm)
{
    SNMPRequest *request = malloc(sizeof(SNMPRequest));

    snmp_sess_init(&request->session);
    request->session.peername = strdup(device->ip);
    request->session.version = SNMP_VERSION_1;
    request->cm = cm;
    request->session.community = (u_char*) device->communities[cm];
    request->session.community_len = strlen((char*)device->communities[cm]);
    request->device_disconnected = &device->disconnected;

    SOCK_STARTUP;
    request->ss = snmp_open(&request->session);

    if(!request->ss)
    {
        snmp_sess_perror("An error occured while openning session SNMP socket", &request->session);
        SOCK_CLEANUP;
        return NULL;
    }

    request->mib_tree = read_mib(FILE_MIB);

    return request;
}

char **SendRequest(SNMPRequest **request, const char *command, char *mode, char **value, unsigned short *size)
{
    char str[SNMP_BUFFER];

    //Init PDU
    (*request)->pdu = snmp_pdu_create(((*request)->cm == COMMUNITY_PUBLIC) ? SNMP_MSG_GET : SNMP_MSG_SET);
    (*request)->anOID_len = MAX_OID_LEN;

    //Mise en forme de la requête
    strcat(strcat(strcpy(str, SNMP_REQUEST_HEAD), command), SNMP_REQUEST_TAIL);

    if(!read_objid(str, (oid*)(&(*request)->anOID),(size_t*)(&(*request)->anOID_len)))
    {
        snmp_perror("SNMP oid parse error");
        SOCK_CLEANUP;
        return NULL;
    }

    //Si PUBLIC, on ajoute l'oid seul
    if((*request)->cm == COMMUNITY_PUBLIC)
        snmp_add_null_var((*request)->pdu, (*request)->anOID, (*request)->anOID_len);
    //Si PRIVATE, on ajoute l'oid ainsi que la valeur et le type de donnée associé
    else if((*request)->cm == COMMUNITY_PRIVATE)
    {
        snmp_add_var((*request)->pdu, (*request)->anOID, (*request)->anOID_len, mode[0], *value);

        //On ajoute la commande de sauvegarde de la configuration au paquet à envoyer
        if(!snmp_parse_oid(".1.3.6.1.4.1.38783.6.0", (*request)->anOID, &(*request)->anOID_len))
        {
          snmp_perror("SNMP oid parse error");
          SOCK_CLEANUP;
          return NULL;
        }

        snmp_add_var((*request)->pdu, (*request)->anOID, (*request)->anOID_len, 'i', "1");
    }

    //Envoi des paquets SNMP au Teracom
    (*request)->status = snmp_synch_response((*request)->ss, (*request)->pdu, &(*request)->response);

    if((*request)->status == STAT_SUCCESS && (*request)->response->errstat == SNMP_ERR_NOERROR)
    {
        char **buffer = NULL;
        unsigned short count;

        for(count = 1, (*request)->vars = (*request)->response->variables ; (*request)->vars ; (*request)->vars = (*request)->vars->next_variable, count++)
        {
            buffer = realloc(buffer, count * sizeof(char *));

            if((*request)->vars->type == ASN_OCTET_STR)
            {

                if(!strcmp(command, "deviceMACAddress"))
                {
                    buffer[count-1] = malloc(18 * sizeof(char));
                    sprintf(buffer[count-1], "%X:%X:%X:%X:%X:%X",
                                (*request)->vars->val.string[0],
                                (*request)->vars->val.string[1],
                                (*request)->vars->val.string[2],
                                (*request)->vars->val.string[3],
                                (*request)->vars->val.string[4],
                                (*request)->vars->val.string[5]);
                }
                else
                {
                    buffer[count-1] = malloc(((*request)->vars->val_len+1) * sizeof(char));
                    strcpy(buffer[count-1],(const char *) (*request)->vars->val.string);
                    buffer[count-1][(*request)->vars->val_len] = '\0';
                }
            }
            else if((*request)->vars->type == ASN_INTEGER)
            {
                buffer[count-1] = malloc(sizeof(long unsigned int));
                sprintf(buffer[count-1], "%lu", *(*request)->vars->val.integer);
            }
            else if((*request)->vars->type == ASN_IPADDRESS)
            {
                buffer[count-1] = malloc(16 * sizeof(char));
                sprintf(buffer[count-1], "%d.%d.%d.%d",
                        (*request)->vars->val.string[0],
                        (*request)->vars->val.string[1],
                        (*request)->vars->val.string[2],
                        (*request)->vars->val.string[3]);
            }

            print_variable((*request)->vars->name, (*request)->vars->name_length, (*request)->vars);
        }

        *size = count-1;
        *(*request)->device_disconnected = 0;

        return buffer;
    }
    else
    {
        if((*request)->status == STAT_SUCCESS)
            fprintf(stderr, "Error in packet, reason: %s\n", snmp_errstring((*request)->response->errstat));
        else if((*request)->status == STAT_TIMEOUT)
            fprintf(stderr, "Timeout: No response from %s.\n", (*request)->session.peername);
        else
            snmp_sess_perror("SNMP session error", (*request)->ss);

        *(*request)->device_disconnected = 1;

        return NULL;
    }
}

void CloseRequest(SNMPRequest **request)
{
    if((*request)->response)
        snmp_free_pdu((*request)->response);
    snmp_close((*request)->ss);

    SOCK_CLEANUP;
}
