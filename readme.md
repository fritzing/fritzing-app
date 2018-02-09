# Fritzing

Ang Fritzing application ay isang Electronic Design Auromation software na madaling pasukin, sakto sa mga manggagawa o hobbyist.ito ay nag ooffer ng isang kakaibang mala buhay na "breadboard", at ang bahaging library na may maraming common na high-level components na magagamit. Ang Fritzing ay nag papadali ng komunikasyon tungkol sa circuits, pati na ang pag lipat nito sa PCB layouts na handa na sa produksyon

* Para sa ibang impormasyon tungkol sa Fritzing at sa mga iba nitong gawain, bisitahin ang[http://fritzing.org](http://fritzing.org). There you can also [download](http://fritzing.org/download) ang bagong labas pra sa lahat ng plataporma at makakuha ng tulong sa pag simula.

* Para mag report ng problema or mag bigay ng suhesyon, gamitin ang [issue tracker](https://github.com/fritzing/fritzing-app/issues) or the [user forum](http://forum.fritzing.org)

* Kung gusto mong tumulong sa development, paki tignan ang [developer instructions](https://github.com/fritzing/fritzing-app/wiki). Ito ay may impormasyon tungkol sa kung paano mag tala at patakbuhin ang Fritzing app sa iilang hakbang lang.

# Folder Structure


* **bins** - Part bins (.fzb, aka part libraries) ay mga coleksyon ng mga parte, lalong-lalo na ang "core" parts bin, at vendor-specific collections.

* **help** - Tapos-dokyumentasyon ng manggagawa ay kasali sa app. Ito ay kailangang mailipay sa website.

* **parts** - Ang lahat ng bahaging depenasyon, kasali ang meta data (.fzp) at graphics (.svg), pati na ang ibang utlity tools. Ito ay itinatago sa ibang repository sa https://github.com/fritzing/fritzing-parts](http://github.com/fritzing/fritzing-parts) at dito lang naka link.

* **pri** - Depenasyon ng submodule ng Qt

* **resources** - Binaries at depenasyon na di dapat ginagalaw ng users, tulad ng fonts, emahe, special parts, etc.

* **sketches** - Halimbawa circuits/sketches ay dala ng aplikasyon

* **src** - Logic ng aplikasyon!

* **tools** - Utility tools para sa pagpapakawala, conbersyon ng bahagi, etc.

* **translations** - Pagsasalin ng lingwahe


# Credits

Ang Fritzing app ay pinapanatili ng Friends-of-Fritzing e.V., isang non-profit na pundasyon na naka base sa Berlin, Germany. Ang proyekto ay lumago sa isang state-funded na proyektong pananaliksik sa [Interaction Design Lab](http://idl.fh-potsdam.de) sa [Potsdam University of Applied Sciences](http://fh-potsdam.de).

Ang core team ay binubuo nila Prof. Reto Wettach, André Knörig, Jonathan Cohen, at Stefan Herman. Maraming [magagaling na tao](http://fritzing.org/about/people/) na nakapag-ambag dito sa buong taon.

Ang Fritzing app ay isinulat sa ibabaw ng [Qt cross-platform framework](http://qt-project.org).


# Licensing


Ang pinagmulang ng code ng Fritzing ay lisensydo sa ilalim ng GNU GPL v3, ang dokumentasyon at part design sa ilalim ng Creative Commons Attribution-ShareALike 3.0 Unported. Ang buonf teksto nitong lisensya ay dala kasama nitong download.

Ito nangangahulugang na makakagawa ka ng sarili mong baryasyon ng Fritzing, hanggat nagbibigay ka ng krideto sa amin at pati sa paglathala sa ilalim ng ngGpl. Katulad ng, pwede mong ilathala ulit ang aming dokumentasyon, hanggat nagbibigay ka ng krideto sa amin at pati na sa paglathala sa ilalim ng kaparehong lesinsya. Pwede kang maglathala ng circuits at diagram na iyong ginawa sa Fritzing na gumagamit sa aming graphics, sa ulit, hanggat nagbibigay ka ng kredito sa amin, at maglathala ka ng iyong gwa sa ilalim sa kaparehong lisensya. Ang kredito au maaring kasing simple lang katulad ng "itong imahe at gwa muola sa Fritzing"


Hanapin ang [our FAQs](http://fritzing.org/faq/) para sa iba pang mga detalye tungkol sa lisensyang ito.
