#ifndef KRONOS_H
#define KRONOS_H

class Kronos
{
  /*
   * Metody         Zwraca              Opis
   * del(x)       - bool(true)        - delay milisekundy
   * delMicro(x)  - bool(true)        - delay mikrosekundy
   * startDel     - bool(true)        - opozniony start (milisekundy)
   * 
   * CNT          - void              - zacznij odliczanie
   * STP          - czas (mili)       - stop odliczanie
   * 
   * ONCE         - bool (true)       - true jezeli raz wykonalo
   * ONCE_RES     - void              - resetuj ONCE
   */ // NAPRAWIONY OVERFLOW m-last > del
   
  public:
  Kronos() : mili(0), rev(true), already(false) {};
  ~Kronos(){};
  bool del (unsigned int delx)
  {
    if (millis()-mili > delx)
    {
      this->mili = millis();
      return true;
    }
    else return false;
  }
  bool delMicro (unsigned int delx)
  {
    if (micros()-mili > delx)
    {
      this->mili = micros();
      return true;
    }
    else return false;
  }
  bool startDel (unsigned int delx)
  {
    if (rev == true)
    {
      this->mili = millis();
      this->rev = false;
      return false;
    }
    if (this->rev == false)
    {
      if (millis()<mili+delx) return false;
      else
      {
        this->rev = true;
        return true;
      }
    }
  }

  void CNT ()
  {
    this->mili = millis();
  }
  unsigned long STP ()
  {
    this->miliNew = millis();
    return (this->miliNew-(this->mili));
  }
  
  bool ONCE ()
  {
	  if (this->already == true) return false;
	  else 
	  {
		  this->already = true;
      return true;
	  }
  }

  void RESET ()
  {
    if (this->already == true) this->already = false;
  }

  
  
  private:
  bool already = false;
  bool rev = true;
  unsigned long mili;
  unsigned long miliNew;
};

#endif
