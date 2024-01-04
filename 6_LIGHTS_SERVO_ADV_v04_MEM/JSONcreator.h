class JSONcreator
{
  public:
  JSONcreator(){}
  ~JSONcreator(){}

    // Foos
    void operator()(char const * field, char const * val)
    { if (this->form.length() == 0) { form+="{\""; form+=field; form+="\":\""; form+=val; form+="\""; }
    else { form+=",\""; form+=field; form+="\":\""; form+=val; form+="\""; }}

    void strVal(char const * field, String val)
    { if (this->form.length() == 0) { form+="{\""; form+=field; form+="\":\""; form+=val; form+="\""; }
    else { form+=",\""; form+=field; form+="\":\""; form+=val; form+="\""; }}
    
    template<typename S, typename V>
    void operator()(S field, V val)
    { if (this->form.length() == 0) { form+="{\""; form+=field; form+="\":"; form+=String(val); }
    else { form+=",\""; form+=field; form+="\":"; form+=String(val); } }
    
  void endJson() { form+="}\n"; }
  void clearJson() { this->form = ""; }
  String giveStr() { return form; }
  char const* giveCstr() { return form.c_str(); }

  private:
  String form = "";
};
