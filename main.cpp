#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <set>
#include <unordered_map>
#include <algorithm>

using namespace std;

/*
            Input: O expresie regulata valida
            Output: DFA

            Am utilizat algoritmul prezentat in urmatoarele surse:
            -     https://www.youtube.com/watch?v=uPnpkWwO9hE
            -     https://www.youtube.com/watch?v=G8i_2CUHP_Y

*/

#define OPERATOR_CONCATENARE '.'
#define OPERATOR_STAR '*'
#define OPERATOR_SAU '|'
#define DESCHIDERE_PARANTEZA '('
#define INCHIDERE_PARANTEZA ')'
#define SIMBOL_FINAL '#'
#define SIMBOL_LAMBDA '&'
#define EROARE "[EROARE] "
#define EXPRESIE_PRELUCRATA '_'

// pentru reprezentarea nodului arborelui de sintaxa
class NodArbore {
public:
    set <unsigned int> firstPos, lastPos;
    bool nullable{};
    char caracter{};
    NodArbore *stanga{}, *dreapta{}, *radacina{};
    bool visited{}; // folosit pentru parcurgerea dfs
};

// reprezentarea FollowPosition conform algoritmului utilizat
class FollowPosition {
public:
    set <unsigned int> lista;
    char elementAlfabet{};
    unsigned int pozitieEelement{};

    FollowPosition(char elementAlfabet, unsigned int pozitieEelement) {
        this->elementAlfabet = elementAlfabet;
        this->pozitieEelement = pozitieEelement;
    }
};

// reprezentarea unei tranzitii pt DFA
class Tranzitie {
public:
    unsigned int stareSursa, stareDestinatie;
    char elementAlfabet;

    Tranzitie(unsigned int stareSursa, unsigned int stareDestinatie, char elementAlfabet){
        this->stareSursa = stareSursa;
        this->stareDestinatie = stareDestinatie;
        this->elementAlfabet = elementAlfabet;
    }
};

// creez DFA ul pe baza tabelului FollowPosition
class DFA {
public:
    set<char> alfabet;
    unsigned int stareInitiala{};
    set <unsigned int> stariFinale;
    vector <set<unsigned int>> stari;   // de exemplu o stare poate fi {1,2,3}. Ii voi asocia un indice ,spre ex 1.
    // Deci starea 1 va avea componentele {1,2,3}

    vector <Tranzitie*> tranzitii;
};

// alfabetul dupa ce validarea alfabetului introdus s-a efectuat
set<char> alfabetFinal;

// va contine caracterele din frunze iar indexul va reprezenta pozitia acesteia in arbore (de la stanga la dreapta)
vector<char> frunzeArbore;

// fiecare pereche de paranteze creeaza un nou nivel de prioritate (0 reprezinta gradul cel mai mic (arorele principal),
// iar prioritatea maxima reprezinta parantezele la nivelul cel mai identat (numarul de stive active) )
vector <stack <NodArbore *>> stivaSubarbori;

// dupa ce creez arborele de sintaxa verific ca acesta sa coincida cu expresia extinsa de la care s-a plecat
string validitateArboreSintaxa;



/** Functii pentru validarea alfabetului de intrare */
bool esteSimbolPredefinit(char x){
    // pentru simbolul lambda nu se face verificare deoarece va fi acceptata introducerea lui
    if (x == DESCHIDERE_PARANTEZA || x == INCHIDERE_PARANTEZA || x == OPERATOR_STAR || x == OPERATOR_SAU ||
        x == OPERATOR_CONCATENARE || x == EXPRESIE_PRELUCRATA) {
        return true;
    }
    return false;
}

bool esteOperator(char x) {
    if(x == OPERATOR_STAR || x == OPERATOR_SAU || x == OPERATOR_CONCATENARE){
        return true;
    }
    return false;
}

bool esteInAlfabet(char x){
    set<char>::iterator itr;
    for (itr = alfabetFinal.begin(); itr != alfabetFinal.end(); itr++){
        if(*itr == x){
            return true;
        }
    }
    return false;
}

bool alfabetValid(const string& alfabet){
    // la alfabet se va adauga si simbolul LAMBDA
    string alfabetExtins = alfabet;
    alfabetExtins.append(1,SIMBOL_LAMBDA);

    for(char x: alfabetExtins){
        if(esteSimbolPredefinit(x)){
            cout << "Alfabetul contine simbolul [" << x << "]! Acest simbol nu este permis!\n";
            return false;
        }
        alfabetFinal.insert(x);
    }
    return true;
}


/** Functii pentru validarea formatului expresiei regex **/

bool corectParantezata(const string& expresie){
    // se verifica daca expresia este corect parantezata
    stack <char> tmp;
    for(auto x:expresie){
        if(x == DESCHIDERE_PARANTEZA){
            tmp.push(DESCHIDERE_PARANTEZA);
            continue;
        }
        if(x == INCHIDERE_PARANTEZA && tmp.top() == DESCHIDERE_PARANTEZA){
            tmp.pop();
            continue;
        }
    }

    return tmp.empty();
}

bool corectPrevalidat(const string& expresie){
    // se verifica daca expresia este corect parantezata si daca contine doar simboluri din alfabetul declarat si
    // operatori
    for(auto x:expresie){
        if(!esteInAlfabet(x) && x != DESCHIDERE_PARANTEZA && x != INCHIDERE_PARANTEZA && x != OPERATOR_STAR
           && x != OPERATOR_CONCATENARE && x != OPERATOR_SAU){
            cout << "\n" << EROARE << "[corectPrevalidat] Simbolul '" << x << "' nu este nici in alfabet si nu este nici operator!\n";
            return false;
        }
    }

    if(!corectParantezata(expresie)){
        cout << "\n" << EROARE << "[expresieValida] Parantezele nu sunt puse corect in expresie!\n";
        return false;
    }

    return true;
}

bool contineSimbolConcatenare(const string& expresie){
    return std::any_of(expresie.begin(),expresie.end(),[](char x){ return x == OPERATOR_CONCATENARE; });
}

bool sablonCorect(const string& expresie, bool contineSimbolConcatenare){
    // este creat un automat care sa verifica daca expresia introdusa este valida

    // reprezinta orice element din alfabet
    const char ELEMENT_ALFABET = '~';

    // sunt indicii pentru stari
    unordered_map<char,unsigned int> indiciElemente;
    indiciElemente.insert(make_pair(OPERATOR_STAR,1));
    indiciElemente.insert(make_pair(OPERATOR_SAU,2));
    indiciElemente.insert(make_pair(DESCHIDERE_PARANTEZA,3));
    indiciElemente.insert(make_pair(INCHIDERE_PARANTEZA,4));
    indiciElemente.insert(make_pair(ELEMENT_ALFABET,5));

    unsigned int stareInitiala = 0;
    set<unsigned int> stari{0,1,2,3,4,5,6};
    set<unsigned int> stariFinale{1,4,5};

    // -1 va reprezinta LIPSA_TRANZITIE de la i la j
    // ultima linie si ultima coloana sunt pentru operatorul '.'
    int reprezentareAFD[7][7] = {   // A va reprezenta orice element din alfabet
            {-1,-1,-1, 3,-1, 5,-1}, // din satrea intiala se poate merge mai departe doar cu '(' si 'A'
            {-1,-1, 2, 3, 4, 5,-1}, // dupa '*' pot urma doar elementele: '|', '(', ')', 'A'
            {-1,-1,-1, 3,-1, 5,-1}, // dupa '|' pot urma doar elementele: '(', 'A'
            {-1,-1,-1, 3,-1, 5,-1}, // dupa '(' pot urma doar elementele: '(', 'A'
            {-1, 1, 2, 3, 4, 5,-1}, // dupa ')' pot urma doar elementele: '*', '|', '(', ')', 'A'
            {-1, 1, 2, 3, 4, 5,-1}, // dupa 'A' pot urma doar elementele: '|', '(', ')', 'A'
            {-1,-1,-1,-1,-1,-1,-1}
    };

    // daca simbolul concatenare este deja continut in expresie se face o alta verificare care sa tina cont si de acesta
    if(contineSimbolConcatenare){
        indiciElemente.insert(make_pair(OPERATOR_CONCATENARE,6));

        // dupa '*' va putea urma si '.'
        reprezentareAFD[1][6] = 6;

        // dupa ')' pot urma doar elementele: '*', '|', ')', '.'
        reprezentareAFD[4][1] = 1;
        reprezentareAFD[4][3] = -1;
        reprezentareAFD[4][5] = -1;
        reprezentareAFD[4][6] = 6;

        // dupa 'A' pot urma doar elementele: '*', '|', ')', '.'
        // 'A' va reprezenta orice element din alfabet
        reprezentareAFD[5][1] = 1;
        reprezentareAFD[5][3] = -1;
        reprezentareAFD[5][5] = -1;
        reprezentareAFD[5][6] = 6;

        // dupa '.' pot urma doar elementele: '(', 'A'
        reprezentareAFD[6][3] = 3;
        reprezentareAFD[6][5] = 5;
    }

    unsigned int stareCurenta = stareInitiala;

    // din starea intiala se poate pleca doar cu DESCHIDERE_PARANTEZA sau cu un element din alfabet
    if (expresie[0] != DESCHIDERE_PARANTEZA && !esteInAlfabet(expresie[0])){
        cout << " \n" << EROARE << "[sablonCorect]. Expresia nu poate incepe cu elementul" << expresie[0];
        return false;
    }

    // se verifica expresia
    int lim = expresie.size();
    for(int i = 0; i < lim; i++){

        char element = expresie[i];

        // se preia indicele corespunzator starii conform elementului citit
        int indiceElement;
        // simbolul lambda il consider ca element din alfabet
        if(esteSimbolPredefinit(element)){
            indiceElement = indiciElemente.find(element)->second;
        }
        else{
            indiceElement = indiciElemente.find(ELEMENT_ALFABET)->second;
        }

        // se merge in starea noua
        stareCurenta = reprezentareAFD[stareCurenta][indiceElement];

        // Daca cu elementul nu curent nu a existat din starea anterioara tranzitie automatul se opreste
        // (-1 reprezinta LIPSA_TRANZITIEI).
        if (stareCurenta == -1 && i > 0){
            cout << " \n" << EROARE << "[sablonCorect]. Dupa elementul '" << expresie[i-1] << "' nu poate urma "
                 << "elementul '" << element << "'";
            return false;
        }
    }

    for(auto stareFinala: stariFinale){
        if(stareCurenta == stareFinala){
            return true;
        }
    }

    return false;
}

bool expresieValida(const string& expresie){

    if (expresie.empty()){
        cout << "\n" << EROARE << "[expresieValida] Expresia este vida !\n";
        return false;
    }

    // se verifica daca expresia este corect parantezata si daca contine doar simboluri din alfabetul declarat si
    // operatori
    if (!corectPrevalidat(expresie)){
        cout << "\n" << EROARE << "[expresieValida] Expresia nu are formatul corect!\n";
        return false;
    }

    // expresia este trecuta printr-un automat a.i. sa se verifica daca are formatul corect
    if (!sablonCorect(expresie,contineSimbolConcatenare(expresie))){
        cout << "\n" << EROARE << "[expresieValida] Expresia nu are formatul corect!\n";
        return false;
    }

    return true;
}

string prefixToInfix(const string& expresiePrefixata) {

    stack<string> stivaSubExpresii;
    int lim = expresiePrefixata.size();

    // expresia este prelucrata de la dreapta spre stanga
    for (int i = lim - 1; i >= 0; i--) {

        // cand este operator , acesta va trebui introdus intre primele 2 elementele ale stivei
        if (esteOperator(expresiePrefixata[i])) {
            // se extrag primele doua elemente de pe stiva
            string element1 = stivaSubExpresii.top();
            stivaSubExpresii.pop();
            string element2 = stivaSubExpresii.top();
            stivaSubExpresii.pop();

            // se face concatenarea
            string tmp;
            tmp.append(1,DESCHIDERE_PARANTEZA);
            tmp.append(element1);
            tmp.append(1,expresiePrefixata[i]);
            tmp.append(element2);
            tmp.append(1,INCHIDERE_PARANTEZA);

            // se adauga noul string pe stiva
            stivaSubExpresii.push(tmp);
            continue;
        }

        // daca simbolul nu este operator atunci este adaugat pe stiva
        stivaSubExpresii.push(string(1, expresiePrefixata[i]));

    }

    // expresia infixata va fi comupsa din subexpresiile din stiva
    string expresieInfixata;
    while (!stivaSubExpresii.empty()){
        string tmp = stivaSubExpresii.top();
        stivaSubExpresii.pop();
        expresieInfixata.append(tmp);
    }
    return expresieInfixata;
}


/** Functii pentru crearea expresiei extinse **/

string adaugaPrecedentaOperatorStar(const string& expresie){
    // In cazul in care operatorul STAR este precedat de un element din alfabet  se adauga paranteze in jurul
    // elementului care il precede si al operatorului (Ex: expresia ab*cd* va fi transformata in a(b*)c(d*) )
    // daca OPERATORUL STAR este precedat de alt element nu se face nimic.
    int lim = expresie.size();
    string tmp;
    for(int i = 0; i < lim; i++){
        if(i > 0 && expresie[i] == OPERATOR_STAR && esteInAlfabet(expresie[i-1])){
            // se elimina elementul precedent
            tmp.pop_back();
            // si se adauga perechea (element precedent,operator *)
            tmp.append(1,DESCHIDERE_PARANTEZA);
            tmp.append(1,expresie[i-1]);
            tmp.append(1,expresie[i]);
            tmp.append(1,INCHIDERE_PARANTEZA);
            continue;
        }
        tmp.append(1,expresie[i]);
    }
    return tmp;
}

string adaugaPrecedentaOperatorSau(const string& expresie){
    int lim = expresie.size();
    string tmp;
    for(int i = 0; i < lim; i++){
        if(expresie[i] == OPERATOR_SAU){
            string aux;
            aux.append(1,OPERATOR_SAU);
            aux.append(1,DESCHIDERE_PARANTEZA);
            aux += adaugaPrecedentaOperatorSau(expresie.substr(i + 1));
            aux.append(1,INCHIDERE_PARANTEZA);

            if(!tmp.empty()) {
                tmp.insert(tmp.begin(),DESCHIDERE_PARANTEZA);
                tmp.append(1,INCHIDERE_PARANTEZA);
                tmp += aux;
                return tmp;
            }
            return aux;
        }
        tmp += expresie[i];
    }

    return tmp;
}

string preiaExpresie(int index, const vector <string> & indexExpresiiPrelucrate){
    string tmp = indexExpresiiPrelucrate[index];
    if(tmp.empty()){
        return tmp;
    }
    if(tmp[0] == EXPRESIE_PRELUCRATA){
        // stoi(tmp.substr(1,tmp.size()-2) reprezinta indexul pentru noua expresie
        return preiaExpresie(stoi(tmp.substr(1,tmp.size()-2)), indexExpresiiPrelucrate);
    }
    return tmp;
}

string creeazaPrecedentaOperator(const string& expresie, bool precedentaOperatorStar){

    // o data ce o expresie este prelucrata ea va primi un index
    vector <string> indexExpresiiPrelucrate;

    // se introduce toata expresia intr-un set nou de paranteze
    string expresieModificata = DESCHIDERE_PARANTEZA + expresie + INCHIDERE_PARANTEZA;

    // impart epresiile in subexpresii (in functie de paranteze)
    // capacitatea initiala pentru nivelul 0 (cel fara paranteze)
    vector <string> stivaSubExpresii(1);
    int lim = expresieModificata.size();
    for(int i = 0; i < lim; i++){

        char element = expresieModificata[i];

        if (element == DESCHIDERE_PARANTEZA){
            // cand o paranteza noua se deschide va creste nivelul de prioritate
            string tmp;
            stivaSubExpresii.push_back(tmp);
            continue;
        }

        if (element == INCHIDERE_PARANTEZA){

            string subExpresie = stivaSubExpresii[stivaSubExpresii.size()-1];
            stivaSubExpresii.pop_back();

            // corespunde nivelului curent (era existenta in expresie)
            stivaSubExpresii[stivaSubExpresii.size()-1].append(1,DESCHIDERE_PARANTEZA);

            string precedentaSubExpresie;

            if(precedentaOperatorStar) {
                // Daca se calculeaza precedenta pentru operatorul star in momentul inchiderii nivelului se verifica daca
                // este o expresie de tipul (...)*  (in paranteza poate fi orice). Daca este o expresie de acest tip,
                // expresia se transforma in ((...)*)
                if((i + 1) < lim && expresieModificata[i+1] == OPERATOR_STAR){
                    precedentaSubExpresie.append(1,DESCHIDERE_PARANTEZA);
                    precedentaSubExpresie.append(adaugaPrecedentaOperatorStar(subExpresie));
                    precedentaSubExpresie.append(1,INCHIDERE_PARANTEZA);
                    precedentaSubExpresie.append(1,OPERATOR_STAR);
                    // la urmatoarea iteratie se va sari operatorul STAR pt ca a fost adaugat acum
                    i++;
                }
                else{
                    // se adauga parantezele pentru precedenta operatorului STAR
                    precedentaSubExpresie = adaugaPrecedentaOperatorStar(subExpresie);
                }
            }
            else{
                // se adauga parantezele pentru precedenta operatorului SAU
                precedentaSubExpresie = adaugaPrecedentaOperatorSau(subExpresie);
            }

            // se creeaza indexul pentru expresie
            indexExpresiiPrelucrate.push_back(precedentaSubExpresie);
            string indexExpresie;
            indexExpresie.append(1,EXPRESIE_PRELUCRATA);
            indexExpresie.append(to_string(indexExpresiiPrelucrate.size() - 1));
            indexExpresie.append(1,EXPRESIE_PRELUCRATA);

            // pe stiva se adauga doar indexul
            stivaSubExpresii[stivaSubExpresii.size()-1] += indexExpresie;

            // corespunde nivelului curent (era existenta in expresie)
            stivaSubExpresii[stivaSubExpresii.size()-1].append(1,INCHIDERE_PARANTEZA);
            continue;
        }

        stivaSubExpresii[stivaSubExpresii.size()-1] += element;
    }

    // La final ar trebui sa mai exista doar expresia de pe nivelul 0.
    if (stivaSubExpresii.empty() || stivaSubExpresii.size() > 1){
        cout << "\n" << EROARE << "[creeazaPrecedentaOperatorSau]! Stiva de precedenta pentru opeartorul SAU nu a fost"
                                  " prelucrata corespunzator!\n";
        exit(-1);
    }

    string expresieIntermediara = stivaSubExpresii[0];
    stivaSubExpresii.pop_back();

    // se face inclocuirea in expresia intermediara (ce cu indecsi)
    string expresiePrelucrata;

    bool prelucrareFinalizata = false;
    while(!prelucrareFinalizata){
        lim = expresieIntermediara.size();
        for (int i = 0; i < lim; i++){
            // daca este intalnim acest simbol va urma un index
            if(expresieIntermediara[i] == EXPRESIE_PRELUCRATA){
                int j = i+1;
                bool indexCalculat = false;
                string indexString;
                // se merge pana se gaseste inchiderea simbolului si se preia indexul cuprins intre
                // ex: daca este _12_ atunci indexul va fi 12
                while(!indexCalculat){
                    indexString += expresieIntermediara[j];
                    j++;
                    if(expresieIntermediara[j] == EXPRESIE_PRELUCRATA){
                        indexCalculat = true;
                        // ulterior la reluare se va continua de la pozitia de dupa preluarea indexului
                        i = j;
                    }
                }

                // se transform indexul in intreg
                int index = stoi(indexString);

                // se preia valoarea expresiei
                expresiePrelucrata.append(preiaExpresie(index,indexExpresiiPrelucrate));

                continue;
            }

            expresiePrelucrata.append(1,expresieIntermediara[i]);
        }

        // daca expresia NU mai contine simbolul EXPRESIE_PRELUCRATA atunci inlocuirea este completa
        if (expresiePrelucrata.find(EXPRESIE_PRELUCRATA) == std::string::npos) {
            prelucrareFinalizata = true;
        }
        else{
            // daca inclocuirea nu este completa se reia inlcouirea dar de data aceasta pentru noua expresie
            expresieIntermediara = expresiePrelucrata;
            expresiePrelucrata = "";
        }
    }

    // se retuneaza expresia fara prima pereche de paranteze adugata la intrarea in functie
    return expresiePrelucrata.substr(1,expresiePrelucrata.size() - 2);
}

string adaugaSimbolConcatenare(const string& expresie){
    string tmp;

    // Se merge pana la lim-1 pt ca se verifica doua cate 2 elemente si se vor face urmatoarele transformari
    // (a poate fi orice element din alfabet):
    //      daca este *a --> *.a
    //      daca este *( --> *.(
    //      daca este )a --> ).a
    //      daca este )( --> ).(
    //      daca este aa --> a.a (sunt oricare 2 simboluri alaturate)
    //      daca este a( --> a.(
    int lim = expresie.size();
    for(int i = 0; i < lim - 1; i++){
        if(
                (expresie[i] == OPERATOR_STAR && esteInAlfabet(expresie[i + 1])) ||
                (expresie[i] == OPERATOR_STAR && expresie[i + 1] == DESCHIDERE_PARANTEZA) ||
                (expresie[i] == INCHIDERE_PARANTEZA && esteInAlfabet(expresie[i + 1])) ||
                (expresie[i] == INCHIDERE_PARANTEZA && expresie[i + 1] == DESCHIDERE_PARANTEZA) ||
                (esteInAlfabet(expresie[i]) && esteInAlfabet(expresie[i + 1])) ||
                (esteInAlfabet(expresie[i]) && expresie[i + 1] == DESCHIDERE_PARANTEZA)
                ){
            tmp.append(1, expresie[i]);
            tmp.append(1,OPERATOR_CONCATENARE);
            continue;
        }

        tmp.append(1, expresie[i]);
    }

    // se adauga si ultimul element din expresie (pt ca s-a mers pana la penultimul la pasul anterior)
    tmp.append(1, expresie[lim - 1]);

    return tmp;
}

string creareExpresieExtinsa(const string& regExp){

    // Se creeaza precedenta pentru operatorul STAR de la stanga spre dreapta.
    // ex: daca expresia introdusa este a(a|bc|cd*)*bcd ea va fi transformata in a((a|bc|c(d*))*)bcd
    string expresieExtinsa = creeazaPrecedentaOperator(regExp,true);

    // Se creeaza precedenta pentru operatorul SAU de la stanga spre dreapta.
    // ex: daca expresia introdusa este (ab|cd|abcd)*|ab ea va fi transformata in (((ab)|((cd)|(abcd)))*)|(ab)
    expresieExtinsa = creeazaPrecedentaOperator(expresieExtinsa, false);

    // Daca expresia nu a avut inserat operatorul de concatenare cesta va fi inserat. Operatorul concatenare va fi
    // adaugat doar dupa operatorul star, paranteza inchisa si dupa simbolul din alfabet.
    // (ex: expresia (a|bc)*ab va deveni (a|(b.c))*.a.b
    if(!contineSimbolConcatenare(regExp)){
        expresieExtinsa = adaugaSimbolConcatenare(expresieExtinsa);
    }

    // se face o reverificare a expresiei procesate
    if(!expresieValida(expresieExtinsa)){
        cout << EROARE << "[creareExpresieExtinsa]! Expresia nu a fost prelucrata corespunzator. Rezultatul nu este valid!";
        exit(-1);
    }


    // expresia finala va fi in forma (...).#
    return DESCHIDERE_PARANTEZA + expresieExtinsa + INCHIDERE_PARANTEZA + OPERATOR_CONCATENARE + SIMBOL_FINAL;
}



/** Functii pentru crearea automatului DFA **/

void incarcaArboreSintaxa(NodArbore *radacina){
    // incarc arborele obtinut intr un string pt a verifica daca corespunde cu expresia extinsa
    // de la care s-a pornit prelucrarea
    if (radacina != nullptr){
        incarcaArboreSintaxa(radacina->stanga);
        validitateArboreSintaxa.push_back(radacina->caracter);
        incarcaArboreSintaxa(radacina->dreapta);
    }
}

bool corectitudineArboreSintaxa(const string& expresieRegExtinsa, string &expArbore){
    // verific daca arborele de sintaxa a fost creat corect (utilizand functia incarcaArboreSintaxa)
    // pe baza expresiei extinse

    for(char i : expresieRegExtinsa) { // extrag parantezele
        if (i != DESCHIDERE_PARANTEZA && i != INCHIDERE_PARANTEZA)
            expArbore.push_back(i);
    }

    if (expArbore != validitateArboreSintaxa) {
        return false;
    }

    return true;
}

set <unsigned int> reunestePozitii(const set <unsigned int>& listaA, const set <unsigned int>& listaB){
    // se face o reuniune a listelor A si B a.i. sa nu se introduca valori duplicat

    set<unsigned int> tmpSet;
    for(auto i: listaA){
        tmpSet.insert(i);
    }
    for(auto i: listaB){
        tmpSet.insert(i);
    }

    return tmpSet;
}

bool creareNodArbore(char element){ // functia principala de creare a unui nod pentru arborele de sintaxa

    if (element == DESCHIDERE_PARANTEZA){
        // cand o paranteza noua se deschide va creste nivelul de prioritate
        stack <NodArbore *> tmp;
        stivaSubarbori.push_back(tmp);
        return true;
    }

    if (element == INCHIDERE_PARANTEZA){ // inainte de a scadea nivelul de prioritate fac legaturile necesare daca exista
        // elemente in stiva pt prioritatea curenta

        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaSubarbori.size() - 1;

        if (!stivaSubarbori[nivelPrioritate].empty()){
            // inchiderea nivelului curent

            // extragem radacina nivelului
            auto *tmp = stivaSubarbori[nivelPrioritate].top();
            stivaSubarbori[nivelPrioritate].pop();
            // eliminam stiva din vectorul de stive
            stivaSubarbori.pop_back();
            nivelPrioritate--;

            // Reunirea cu nivelul precedent. Nivelul precedent fiind mereu nivel superior va deveni radacina pentru
            // radacina principala a nivelui curent.
            if (!stivaSubarbori[nivelPrioritate].empty()){

                // se preia radacina nivelui si se ataseaza pe partea dreapta noul fiu
                auto *radacinaNivel = stivaSubarbori[nivelPrioritate].top();

                // pe nivelul precedent radacina nu poate fi decat un nod de tip CONCATENARE sau un nod de tip SAU
                if(radacinaNivel->caracter != OPERATOR_CONCATENARE && radacinaNivel->caracter != OPERATOR_SAU){
                    cout << EROARE << "[creareNodArbore]. radacina a nivelului precedent este [" << radacinaNivel->caracter
                         << "]. Nu se poate atasa fiu pe dreapta!";
                    return false;
                }


                // facem legatura
                radacinaNivel->dreapta = tmp;
                tmp->radacina = radacinaNivel;

                // Atat operatorul SAU cat si operatorul CONCATENARE trebuie sa aiba ambii fii legati in acest pas.
                if(radacinaNivel->stanga == nullptr || radacinaNivel->dreapta == nullptr){
                    cout << EROARE << "[creareNodArbore]. Unul sau ambii fii este null. Nu se poate continua.";
                    return false;
                }

                // Setam valorile nullable, firstpos si lastpos in functie de tipul de operator.

                if(radacinaNivel->caracter == OPERATOR_CONCATENARE){
                    // setare nullable
                    radacinaNivel->nullable = radacinaNivel->stanga->nullable && radacinaNivel->dreapta->nullable;

                    // setare firstpos
                    if(radacinaNivel->stanga->nullable){
                        radacinaNivel->firstPos = reunestePozitii(radacinaNivel->stanga->firstPos,radacinaNivel->dreapta->firstPos);
                    }
                    else{
                        radacinaNivel->firstPos = radacinaNivel->stanga->firstPos;
                    }

                    // setare lastpos
                    if(radacinaNivel->dreapta->nullable){
                        radacinaNivel->lastPos = reunestePozitii(radacinaNivel->stanga->lastPos,radacinaNivel->dreapta->lastPos);
                    }
                    else{
                        radacinaNivel->lastPos = radacinaNivel->dreapta->lastPos;
                    }

                    return true;
                }


                // aici nu mai poate ajunge decat operatorul SAU

                // setare nullable
                radacinaNivel->nullable = radacinaNivel->stanga->nullable || radacinaNivel->dreapta->nullable;

                // setare firstpos
                radacinaNivel->firstPos = reunestePozitii(radacinaNivel->stanga->firstPos,radacinaNivel->dreapta->firstPos);

                // setare lastpos
                radacinaNivel->lastPos = reunestePozitii(radacinaNivel->stanga->lastPos,radacinaNivel->dreapta->lastPos);

                return true;
            }

            stivaSubarbori[nivelPrioritate].push(tmp);
            return true;
        }

        // eliminam stiva din vectorul de stive
        stivaSubarbori.pop_back();
        return true;
    }


    // setarea unor valori intiale pentru nodul curent
    auto *nodCurent = new NodArbore();
    nodCurent->nullable = false;
    nodCurent->caracter = element;
    nodCurent->stanga = nullptr;
    nodCurent->dreapta = nullptr;
    nodCurent->radacina = nullptr;

    if (element == OPERATOR_STAR || element == OPERATOR_CONCATENARE || element == OPERATOR_SAU){

        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaSubarbori.size() - 1;

        // Stiva nu poate fi goala atunci cand pe acelasi nivel vine un operator.
        if (stivaSubarbori[nivelPrioritate].empty()){
            cout << EROARE << "[creareNodArbore]. Stiva este goala pentru operatorul [" << element << "]!";
            return false;
        }

        // in stiva top() va fi radacina nivelului curent
        NodArbore *radacinaNivel = stivaSubarbori[nivelPrioritate].top();
        stivaSubarbori[nivelPrioritate].pop();

        // daca este nod star ii setez valorile nullable, firstpos si lastpos
        if(element == OPERATOR_STAR){
            nodCurent->nullable = true;
            nodCurent->firstPos = radacinaNivel->firstPos;
            nodCurent->lastPos = radacinaNivel->lastPos;
        }

        // se face legatura cu radacina anterioara a nivelului (prin conventie operatorul STAR va avea legatura doar pe
        // stanga)
        nodCurent->stanga = radacinaNivel;
        radacinaNivel->radacina = nodCurent;

        // dupa ce l-am legat el va fi noua radacina a nivelului
        stivaSubarbori[nivelPrioritate].push(nodCurent);
        return true;
    }

    if (esteInAlfabet(element) || element == SIMBOL_FINAL){
        frunzeArbore.push_back(element);
        nodCurent->firstPos.insert(frunzeArbore.size());
        nodCurent->lastPos.insert(frunzeArbore.size());

        // Cate stive sunt active la acest moment. Fiecare stiva corespunde unui nivel de identare al parantezelor.
        unsigned int nivelPrioritate = stivaSubarbori.size() - 1;

        // Inainte unui element din alfabet/SIMBOL_FINAL nu poate exista decat operatorul concatenare
        // (valabil doar pentru expresia extinsa).
        if (!stivaSubarbori[nivelPrioritate].empty()){
            // il leg de radacina nivelului (radacina nivelului este elementul top al stivei)
            NodArbore *radacinaNivel = stivaSubarbori[nivelPrioritate].top();
            radacinaNivel->dreapta = nodCurent;
            nodCurent->radacina = radacinaNivel;
            // nu mai este agaugat in stiva dearece este frunza si deja a fost legat de radacina

            if(radacinaNivel->caracter == OPERATOR_CONCATENARE){
                // setare nullable
                radacinaNivel->nullable = radacinaNivel->stanga->nullable && radacinaNivel->dreapta->nullable;

                // setare firstpos
                if(radacinaNivel->stanga->nullable){
                    radacinaNivel->firstPos = reunestePozitii(radacinaNivel->stanga->firstPos,radacinaNivel->dreapta->firstPos);
                }
                else{
                    radacinaNivel->firstPos = radacinaNivel->stanga->firstPos;
                }

                // setare lastpos
                if(radacinaNivel->dreapta->nullable){
                    radacinaNivel->lastPos = reunestePozitii(radacinaNivel->stanga->lastPos,radacinaNivel->dreapta->lastPos);
                }
                else{
                    radacinaNivel->lastPos = radacinaNivel->dreapta->lastPos;
                }

                return true;
            }

            return true;
        }

        // daca inainte elementului nu a existat nici un operator ==> elementul este adaugat in stiva
        stivaSubarbori[nivelPrioritate].push(nodCurent);
        return true;

    }

    return false;
}

NodArbore *creareArboreSintaxa(string expresieRegExtinsa){

    // initial adaug stiva pentru nivelul 0 (pentru elementele care nu sunt incluse intre paranteze)
    stack <NodArbore *> tmp;
    stivaSubarbori.push_back(tmp);

    int lim = expresieRegExtinsa.size();

    for (int i = 0; i < lim; i++){ // parcurg expresia extinsa si creez arborele de sintaxa
        if (!creareNodArbore(expresieRegExtinsa[i])) { // creez nodurile pt fiecare element
            cout << "\n" << EROARE << "[creareArboreSintaxa] la crearea nodului " << i << " !\n";
            exit(-1);
        }
    }

    // Radacina arborelui va fi elementul din stiva de pe nivelul 0 (va exista doar nivelul 0, iar pe acest nivel va fi
    // doar un singur element dupa prelucrarile anterioare).
    if (stivaSubarbori[0].empty() || stivaSubarbori[0].size() > 1){
        cout << "\n" << EROARE << "[creareArboreSintaxa]! Stiva de nivel 0 este vida sau are mai mult de 1 element!\n";
        exit(-1);
    }

    // se extrage radacina si se elimina si stiva de nivel 0
    NodArbore *radacinaArbore = stivaSubarbori[0].top();
    stivaSubarbori[0].pop();
    stivaSubarbori.pop_back();

    // verific corectitudinea arborelui de sintaxa creat
    incarcaArboreSintaxa(radacinaArbore);
    string tmpAug;

    if (!corectitudineArboreSintaxa(expresieRegExtinsa, tmpAug)){
        cout << "\nArborele de sintaxa nu este creat corect !\n";
        cout <<"\nExpresie extinsa:  " << tmpAug << "\n";
        cout <<"\nArbore sintaxa:       " << validitateArboreSintaxa << "\n";
        exit(-1);
    }

    return radacinaArbore;
}

void aplicaDfs(NodArbore *nod, vector <FollowPosition*>& fPos){

    nod->visited = true;

    vector<NodArbore*> fii;
    if(nod->stanga != nullptr) {
        fii.push_back(nod->stanga);
    }
    if(nod->dreapta != nullptr){
        fii.push_back(nod->dreapta);
    }

    for (auto fiu : fii) {
        if (!fiu->visited) {
            aplicaDfs(fiu,fPos);
        }
    }

    // dupa revenirea din recursivitate se calculeaza followposition pentru nodurile CONCATENARE si nodurile STAR
    if(nod->caracter == OPERATOR_CONCATENARE){
        for(auto i : nod->stanga->lastPos){
            fPos[i-1]->lista = reunestePozitii(fPos[i-1]->lista,nod->dreapta->firstPos);
        }
        return;
    }

    if(nod->caracter == OPERATOR_STAR){
        for(auto i : nod->lastPos){
            fPos[i-1]->lista = reunestePozitii(fPos[i-1]->lista,nod->firstPos);
        }
    }
}

vector <FollowPosition*> calculeazaFollowPosition(NodArbore *radacina, const string& expresieRegExtinsa){

    // Adauga valorile intiale in tabel. Intitial va contine pe fiecare positie caracterul din alfabet si pozitia frunzei
    // sale in arbore.
    int lim = frunzeArbore.size();
    vector <FollowPosition*> fPos(lim);
    for(int i = 0; i < lim; i++) {
        fPos[i] = new FollowPosition(frunzeArbore[i], i + 1);
    }

    // calculeaza restul valorilor (listele de followposition)
    aplicaDfs(radacina, fPos);

    return fPos;
}

bool esteStareNoua(const vector<set<unsigned int>> & listaStari, const set<unsigned int> & stareNoua, unsigned int& stareEchivalenta){
    int lim = listaStari.size();
    for(int i = 0; i < lim; i++){
        if(stareNoua == listaStari[i]){
            stareEchivalenta = i;
            return false;
        }
    }
    return true;
}

set<unsigned int> calculeazaPozitiiStareCurenta(const set<unsigned int>& listaA, const set<unsigned int>& listaB){
    // returneaza elementele din B care se afla in A
    set <unsigned int> tmp;
    for(int i : listaB){
        for(int j : listaA){
            if(i == j){
                tmp.insert(i);
            }
        }
    }

    return tmp;
}

DFA* creareDFA(NodArbore *radacina, vector <FollowPosition*>& fPos){
    // functia principala de creare a DFA ului

    // pt fiecare element unic din alfabet aflu pozitiile din arborele de sintaxa
    // (ex: pt expresia a.b.a ==> elementul a va avea pozitiile {1,3} in arborele de sintaxa, iar elementul b va avea lista {2})
    unordered_map<char, set<unsigned int>> hashStari;
    int lim = frunzeArbore.size();
    for(int i = 0; i < lim; i++){

        if (hashStari.find(frunzeArbore[i]) == hashStari.end()){
            set<unsigned int> tmp;
            tmp.insert(i + 1);
            hashStari.insert(make_pair(frunzeArbore[i],tmp));

            continue;
        }

        // Cheia exista. Mai adauga un index la valoarea veche.
        hashStari.find(frunzeArbore[i])->second.insert(i + 1);
    }


    DFA* dfa = new DFA();
    dfa->alfabet = alfabetFinal;
    // dfa-ul nu va contine tranzitii lambda
    dfa->alfabet.erase(SIMBOL_LAMBDA);

    // Creez starea initiala. Starea intiala va fi firstpos de radacina.
    dfa->stareInitiala = 0;
    set <unsigned int> tmp(radacina->firstPos.begin(),radacina->firstPos.end());
    dfa->stari.push_back(tmp);

    queue <pair<unsigned int, set<unsigned int>>> coadaStariNoi; // folosesc o coada pt a afla cand nu mai am stari noi
    // primul int va fi indexul starii, iar al doilea element va fi starea
    coadaStariNoi.push(make_pair(dfa->stareInitiala,dfa->stari[dfa->stareInitiala])); // adaug starea initiala

    while (!coadaStariNoi.empty()){ // cat timp am stari noi

        int indexStareCurenta = coadaStariNoi.front().first;
        set<unsigned int> indiciStareCurenta = coadaStariNoi.front().second;
        coadaStariNoi.pop();

        // pt fiecare element al alfabetului, aflu tranzitiile pt starea curenta la prelucrare
        for (char x : dfa->alfabet){

            // pt fiecare element unic din alfabet preiau pozitiile din arborele de sintaxa
            // (ex: pt expresia a.b.a ==> elementul a va avea pozitiile {1,3} in arborele de sintaxa, iar elementul b va avea lista {2})
            if (hashStari.find(x) == hashStari.end()) {
                cout << EROARE << "[creareDFA]. Cheia [" << x << "] este inexistenta!";
                exit(-1);
            }

            // de ex daca alementul a apare pe 4 pozitii in frunzele arborelui ( ex: a.b.c.a.a.c.a ==> a = {1,4,5,7} )
            // iar starea curenta este {1,2,3,4}, atunci pozitiile din starea curenta pentru 'a' vor fi {1,4}
            set<unsigned int> indiciCaracter = calculeazaPozitiiStareCurenta(hashStari.find(x)->second, indiciStareCurenta);

            // Creez noua stare. Noua stare va fi reuniunea followpos-urilor din indiciCaracter.
            // pt exemplul de mai sus daca a = {1,4} atunci noua stare pt care din starea curenta se va ajunge cu
            // caracterul 'a'(adica tarnzitia 'a') va fi follopos(1) reunit cu followpos(4)
            // Reuniunea se va face folosind un set a.i. se vor evita automat duplicatele a.i. e de ajuns sa se adauga
            // fiecare element, nefiind necesara o alta verificare.
            set<unsigned int> indiciStareNoua;
            for (auto indice : indiciCaracter) {
                for(auto elem : fPos[indice - 1]->lista){
                    indiciStareNoua.insert(elem);
                }
            }

            // daca starea nou creata are elemente si nu exista deja va fi adaugata in lista de stari
            unsigned int stareEchivalenta = 0; // verific cu ce stare este echivalenta in caz de echivalenta
            if(!indiciStareNoua.empty() && esteStareNoua(dfa->stari, indiciStareNoua, stareEchivalenta)){
                // adauga starea in dfa
                dfa->stari.push_back(indiciStareNoua);
                // adauga stare noua o adaug in coada pt prelucrare
                coadaStariNoi.push(make_pair(dfa->stari.size() - 1,indiciStareNoua));
                // va fi echivalenta cu ea insasi
                stareEchivalenta = dfa->stari.size() - 1;
            }

            // creez tranzitia si o adaug in DFA doar daca am o stare noua nevida
            if(!indiciStareNoua.empty() ){
                dfa->tranzitii.push_back(new Tranzitie(indexStareCurenta,stareEchivalenta,x));
            }
        }
    }

    // Indicele starii finale va fi indicele frunzei care contine SIMBOLU_STARE_FINALA (acesta va fi ultimul index din
    // vectorul de frunze).
    int indiceStareFinala = frunzeArbore.size();

    // Starile finale vor fi starile care contin indicele 'indiceStareFinala'
    lim = dfa->stari.size();
    for (int i = 0; i < lim; i++){
        for (int idx : dfa->stari[i]){
            if (idx == indiceStareFinala){
                dfa->stariFinale.insert(i);
                break;
            }
        }
    }

    return dfa;

}


/** Functii pentru afisarea automatelor **/

void afisareDFA(DFA* &dfa){

    //afisare  consola
    cout << "\n=======================\nAfisare DFA\n=======================\n";
    cout << "\nStare initiala: " << dfa->stareInitiala << "\n";
    cout << "\n-----------------\nNr stari finale: " << dfa->stariFinale.size() << "\n";
    cout << "\nStari Finale: ";
    for (auto i : dfa->stariFinale)
        cout << i << " ";

    cout << "\n-----------------\n\nNr elemente alfabet: " << dfa->alfabet.size() << "\n";
    cout << "\nAlfabet: ";
    for (auto x : dfa->alfabet)
        cout << x << " ";

    cout << "\n\n-----------------\nNr stari: " << dfa->stari.size() << "\n";
    cout << "\nStari:\n-------------\n";
    int lim = dfa->stari.size();
    for(int i = 0; i < lim; i++){
        cout << i << ": { ";
        for (auto x : dfa->stari[i]) {
            cout << x << " ";
        }
        cout << "}\n";
    }
    cout <<"\n----------------\n";
    cout << "\nNumar Tranzitii: " << dfa->tranzitii.size() << "\n";
    cout << "\nTranzitii:\n";
    for(auto tranzitie : dfa->tranzitii) {
        cout << tranzitie->stareSursa << " " << tranzitie->elementAlfabet << " " << tranzitie->stareDestinatie << "\n";
    }
    cout << "\n\n";


    //export date in fisier
    ofstream fout;
    fout.open("dfa_out.txt");

    fout << dfa->stari.size() << "\n";
    for(int i = 0; i < dfa->stari.size(); i++) {
        fout << i << " ";
    }
    fout << "\n";

    fout << dfa->alfabet.size() << "\n";
    for (auto x : dfa->alfabet) {
        fout << x << " ";
    }
    fout <<"\n";

    fout << dfa->stareInitiala << "\n";
    fout << dfa->stariFinale.size() << "\n";
    for (auto x: dfa->stariFinale) {
        fout << x << " ";
    }
    fout <<"\n";

    fout << dfa->tranzitii.size() << "\n";
    for(auto tranzitie : dfa->tranzitii) {
        fout << tranzitie->stareSursa << " " << tranzitie->elementAlfabet << " " << tranzitie->stareDestinatie << "\n";
    }
}

int main(){

    /*
     * Input:
     *  true: daca este introdusa o expresie in forma infixata / false: daca este introdusa in forma prefixata
     *  alfabetul
     *  expresia
     * */

    string numeFisier = "date.txt";

    ifstream fin;
    fin.open(numeFisier);
    if (!fin){
        cout << "\n" << EROARE << "Deschiderea fisierului [ " << numeFisier << " ] nu a reusit!\n";
        exit(-1);
    }

    // pentru a sti daca expresia este infixata sau prefixata
    string esteExpresieInfixata;
    fin >> esteExpresieInfixata;

    string alfabet;
    fin >> alfabet;
    cout << "\nAlfabetul introdus este: " << alfabet << "\n";
    if (!alfabetValid(alfabet)){
        cout << "\n" << EROARE << "Alfabetul introdus [ " << alfabet << " ] nu este valid !\n";
        exit(-1);
    }

    string regExp;
    fin >> regExp;
    cout << "\nExpresia introdusa este: " << regExp << "\n";
    if(esteExpresieInfixata != "true"){
        cout << "\nExpresia introdusa este in forma prefixata. Se face conversia la forma infixata.\n";
        regExp = prefixToInfix(regExp);
        cout << "\nForma infixata a expresiei introduse este [ " << regExp << " ]\n";
    }
    if (!expresieValida(regExp)){
        cout << "\n" << EROARE << "Expresia introdusa [ " << regExp << " ] nu este valida !\n";
        exit(-1);
    }

    string expresieExtinsa = creareExpresieExtinsa(regExp);
    cout << "\nExpresia extinsa este: " << expresieExtinsa << "\n";

    NodArbore *radacinaArboreSintaxa = creareArboreSintaxa(expresieExtinsa);

    vector <FollowPosition*> fPos = calculeazaFollowPosition(radacinaArboreSintaxa, expresieExtinsa);

    DFA* dfa = creareDFA(radacinaArboreSintaxa, fPos);

    afisareDFA(dfa);

    return 0;
}
