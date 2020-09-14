//
//  main.cpp
//  project4
//
//  Created by Cindi Dong on 5/29/19.
//  Copyright © 2019 Cindi Dong. All rights reserved.
//
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
using namespace std;

struct Bucket
{
    string c;
    int index;
    Bucket* next;
};

class hashTable
{
public:
    hashTable(int size) : table(size)
    {
        tableSize = size;
        prevInsert = "";
    }
    ~hashTable()
    {
        for (int i = 0; i < tableSize; i++)
        {
            //if there is no bucket, don't delete
            if (table[i] == nullptr)
                continue;
            //deleting the linked list
            if (table[i]->next != nullptr)
            {
                Bucket* n = table[i]->next;
                Bucket* nNext;
                while (n != nullptr)
                {
                    nNext = n->next;
                    delete n;
                    n = nNext;
                }
            }
            delete table[i];
        }
    }
    void insert(string s, int position)
    {
        int indexOfTable = hashFunc(s, tableSize);
        //this prevents s from being added if the previous value inserted was also s
        if (prevInsert == s)
            return;
        if ((table[indexOfTable]) == nullptr)
        {
            Bucket *b = new Bucket;
            table[indexOfTable] = b;
            table[indexOfTable]->c = s;
            table[indexOfTable]->index = position;
            table[indexOfTable]->next = nullptr;
            prevInsert = s;
        }
        else
        {
            Bucket* p = table[indexOfTable];
            //iterate to the last bucket
            while (p->next != nullptr)
                p = p->next;
            Bucket *b = new Bucket;
            b->c = s;
            b->index = position;
            b->next = nullptr;
            p->next = b;
            prevInsert = s;
         }
    }
    bool search(string s)
    {
        int index = hashFunc(s, tableSize);
        if (table[index] == nullptr)
            return false;
        if (table[index]->c == s)
        {
            return true;
        }
        Bucket* p = table[index];
        while (p != nullptr)
        {
            if (p->c == s)
                return true;
            p = p->next;
        }
        return false;
    }
    //same as above, but returns a bucket
    Bucket* search(string s, int idx)
    {
        int index = hashFunc(s, tableSize);
        if (table[index] == nullptr)
            return nullptr;
        if (table[index]->c == s && table[index] -> index == idx)
        {
            return table[index];
        }
        Bucket* p = table[index];
        while (p != nullptr)
        {
            if (p->c == s && p->index == idx)
                return p;
            p = p->next;
        }
        return nullptr;
    }
    Bucket* getBucket(string s)
    {
        int index = hashFunc(s, tableSize);
        if (table[index] == nullptr)
            return nullptr;
        if (table[index]->c == s)
        {
            return table[index];
        }
        Bucket* p = table[index];
        while (p != nullptr)
        {
            if (p->c == s)
                return p;
            p = p->next;
        }
        return nullptr;
    }
    int getIndex(Bucket* p)
    {
        return p->index;
    }
    string getValue(Bucket* p)
    {
        return p->c;
    }
private:
    int hashFunc(string s, int size)
    {
        unsigned int h = 2166136261U;
        for (int i = 0; i < s.size(); i++)
        {
            char c = s.at(i);
            h += c;
            h *= 16777619;
        }
        return h%size;
    }
    int tableSize;
    string prevInsert;
    vector<Bucket*> table;
};

void createDiff(istream& fold, istream& fnew, ostream& fdiff);
//This function takes the contents of two files, A and A', and produces a diff file containing instructions for
//converting A into A'. Each day, Small-Mart will use this function at their corporate headquarters to create a new diff
//file that they will then send to all the devices.

bool applyDiff(istream& fold, istream& fdiff, ostream& fnew);
//This function takes the content of a file A and a diff file, and will apply the instructions in the diff file to
//produce a new file A'. Each day, every device will use this function to update the previous day's inventory file.

bool getInt(istream& inf, int& n)
{
    char ch;
    if (!inf.get(ch)  ||  !isascii(ch)  ||  !isdigit(ch))
        return false;
    inf.unget();
    inf >> n;
    return true;
}

bool getCommand(istream& inf, char& cmd, int& length, int& offset)
{
    if (!inf.get(cmd))
    {
        cmd = 'x';  // signals end of file
        return true;
    }
    char ch;
    switch (cmd)
    {
        case 'A':
            return getInt(inf, length) && inf.get(ch) && ch == ':';
        case 'C':
            return getInt(inf, length) && inf.get(ch) && ch == ',' && getInt(inf, offset);
        case '\r':
        case '\n':
            return true;
    }
    return false;
}

//This function is used to calcuate the index and the length of the maximum string that can be copied.
int longestCopyCounter(hashTable& h, string temp, string old, string next, int j, int& index)
{
    int depth = 0;
    Bucket* p = h.getBucket(temp);
    int maxCopyCounter = temp.size();
    while (p != nullptr)
    {
        //ends after 10 loops
        if (depth > 10)
            break;
        if (p->c != temp)
        {
            p = p->next;
            continue;
        }
        int copyCounter = temp.size();
        int copyIndex = h.getIndex(p);
        int k = h.getIndex(p) + temp.size();
        //keep going until the characters don't match up or reaches the end of a string
        while (k < old.size() && j+copyCounter < next.size() && old.at(k) == next.at(j+copyCounter))
        {
            copyCounter++;
            k++;
        }
        if (maxCopyCounter <= copyCounter)
        {
            maxCopyCounter = copyCounter;
            index = copyIndex;
        }
        p = p->next;
        depth++;
    }
    return maxCopyCounter;
}
void createDiff(istream& fold, istream& fnew, ostream& fdiff)
//This function takes the contents of two files, A and A', and produces a diff file containing instructions for
//converting A into A'. Each day, Small-Mart will use this function at their corporate headquarters to create a new diff
//file that they will then send to all the devices.
{
    
    string old = "";
    string next = "";
    char tempC;
    //getting input
    while (fold.get(tempC))
    {
        old += tempC;
    }
    while (fnew.get(tempC))
    {
        next += tempC;
    }
    //make sure load factor is not over 0.6
    int sizeOfHashTable = old.size()/0.6;
    hashTable h(sizeOfHashTable);
    int n = 10;
    if (old.size() >= 100000)
        n = 32;
    int addIndex = 0;
    int copyIndex = 0;
    int i = 0, j = 0;
    int addCounter = 0;
    int copyCounter = 0;
    //inserting into hash table
    for (i = 0; i < next.size() - n; i++)
    {
        string temp;
        for (int j = i; j < i + n; j++)
        {
            if (j >= old.size())
                break;
            temp += old.at(j);
        }
        if (temp.size() < n)
            break;
        h.insert(temp, i);
    }
    while (j < next.size())
    {
        string test = next.substr(j, n);
        if (h.search(test))
        {
            //if addCounter has made it here, it is done being added and needs to be printed
            if (addCounter > 0)
            {
                fdiff<<"A"<<addCounter<<":"<<next.substr(addIndex, addCounter);
                addCounter = 0;
            }
            //find the longest string to be copied
            copyCounter = longestCopyCounter(h, test, old, next, j, copyIndex);
            j = j + copyCounter;
            if (copyCounter > 0)
            {
                fdiff<<"C"<<copyCounter<<","<<copyIndex;
                copyCounter = 0;
            }
        }
        else
        {
            if (addCounter == 0)
            {
                addIndex = j;
            }
            addCounter++;
            j++;
        }
    }
    //if the charaters at the end don't match, just add them
    if (addCounter > 0)
        fdiff<<"A"<<addCounter<<":"<<next.substr(addIndex, addCounter);
    //if the characters at the end match, just copy them
    if (copyCounter > 0)
        fdiff<<"C"<<copyCounter<<":"<<copyIndex;
}

bool applyDiff(istream& fold, istream& fdiff, ostream& fnew)
//This function takes the content of a file A and a diff file, and will apply the instructions in the diff file to
//produce a new file A'. Each day, every device will use this function to update the previous day's inventory file.
{
    string old = "";
    string diff = "";
    char tempC;
    while (fold.get(tempC))
    {
        old += tempC;
    }
    char cmd = ' ';
    int length = 0;
    int offset = 0;
    while (getCommand(fdiff, cmd, length, offset) == true)
    {
        if (cmd == 'A')
        {
            for (int i = 0; i < length; i++)
            {
                char k;
                fdiff.get(k);
                fnew<<k;
            }
        }
        if (cmd == 'C')
        {
            string s = "";
            //checks if the copy command goes off the end of the string
            if (offset+length > old.size())
                return false;
            for (int i = offset; i < offset + length; i++)
            {
                s += old.at(i);
            }
            fnew<<s;
        }
        if (cmd == 'x')
        {
            return true;
        }
    }
    return false;
}

int main()
{
    1, 2, 2*2, 2*2*2, 2*2*2*2, 32,
    32, 16, 8, 4, 2, 1
    while (k != 0)
        k = k/2;
    return 0;
}
