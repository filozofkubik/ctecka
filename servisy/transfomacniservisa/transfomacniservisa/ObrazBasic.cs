using System;
using System.Collections.Generic;
using System.Text;

namespace transfomacniservisa
{

    public  class   ObrazBasic : Itransformace
    {
        protected string nazevVstupnihoadresare;
        protected string nazevVystupnihoadreare;
        protected string nazevSouboru;
    
    


        public void Inicializuj(string nazevVstupnihoadresare, string nazevVystupnihoadreare, string nazevSouboru)
        {
            this.nazevVstupnihoadresare = nazevVstupnihoadresare;
            this.nazevVystupnihoadreare = nazevVystupnihoadreare;
            this.nazevSouboru = nazevSouboru;
        }

        public virtual void ZpracujSoubor()
        {
            throw new NotImplementedException();
        }
    }
}
