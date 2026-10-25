using System;
using System.Collections.Generic;
using System.Text;

namespace transfomacniservisa
{

        public interface Itransformace
        {
            // Vlastnost, kterou musí třída mít
          

            // Metody, které musí třída implementovat
        public  void Inicializuj(string nazevVstupnihoadresare, string nazevVystupnihoadreare, string nazevSouboru);
        public   void ZpracujSoubor();
         /// <summary>
        /// Inicializuje instanci rozhraní Itransformace.
        /// </summary>
        /// <param name="nazevVstupnihoadresare">Název vstupní složky.</param>
        /// <param name="nazevVystupnihoadreare">Název výstupní složky.</param>
        /// <param name="nazevSouboru">Název souboru.</param>
        //Itranformace(string nazevVstupnihoadresare,string nazevVystupnihoadreare,string nazevSouboru);
        }
    }

