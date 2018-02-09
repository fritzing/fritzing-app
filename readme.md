# Fritzing

Ang Fritzing nga aplikasyon kay usa ka Electronic Design Automation software na pwede ra makasud ug sayun, mas maayo para sa manghimuay ug mga naglingawlingaw ra. Naghatag ni ug lahi nga murag tinuod nga "breadboard" sa talan-awon, ug ang parts libray nga adunay daghang nindot ug maayung mga components. Fritzing naghatag ug kasayun sa pakig sinabot sa circuit, ug ang pagbalhin niini sa PCB layout nga andam na sa produksyon. Kini kay sikat na sa Arduino ug sa Raspberry Pi na manggamitay, ug halapad na ang nigamit niini sa para sa edukasyon ug sa mamugnaong pag hunahuna.

* Para sa uban pang impormasyon bahin sa Fritzing ug sa uban niining mga kalihokan, bisitaha ang [http://fritzing.org](http://fritzing.org). Diri usab ika maka [download](http://fritzing.org/download) sa bag-ong plataporma ug mga tabang sa pag sugod.

* Para sa pag report ug problema o pag hatag ug suhesyon para sa pag-uswag, gamita ang [issue tracker](https://github.com/fritzing/fritzing-app/issues) or the [user forum](http://forum.fritzing.org)

* Kung ganahan ka makatabang sa pagpalambo, palihug tan-aw sa [developer instructions](https://github.com/fritzing/fritzing-app/wiki). Aduna pod diri ang mga impormasyon bahin sa kung unsaon pag han-ay ug pag padagan sa Fritzing app sa kadiyot rang paagi.


# Folder Structure

* **bins** - Part bins (.fzb, aka part libraries) kay mga koleksyon sa mga parte, labi na kadtong "core" nga parts bin, ug vendor-specific na mga koleksyon.

* **help** - Pagkahuman-documentason sa nag gamit kauban sa app. Kini kinahanglan gayud nga ma balhin sa website.


* **parts** - Tanang part definitions, kauban ang meta data (.fzp) ug fraphics (.svg), ug ang mga utility tools. Kini kay gihipos lahi sa repository didto sa [https://github.com/fritzing/fritzing-parts](http://github.com/fritzing/fritzing-parts) ug na link ra diri.


* **pri** - Depenasyon sa submodule para sa Qt

* **resources** - Binaries ug depenasyon na kinahanglang dili hilabtan sa mga users, pariha sa fonts, larawan, special parts, etc.

* **sketches** - Pananglitan circuit/sketches gipadala kauban ang aplikasyon

* **src** - Lohika sa appication!

* **tools** - Utility tools para sa paggama sa pagpagawas, pag pagkakabig sa mga bahin, etc.

* **translations** - Paghubad sa pinulungan

# Credits

Ang Fritzing app kay gimintinar sa Friends-of-Fritzing e.V., usa ka non-profit foundation nga naka base sa Berlin, Germany. Ang proyekto kay nag tubo gawas sa usa ka state-funded research nga proyekto sa [Interaction Design Lab](http://idl.fh-potsdam.de) didto sa [Potsdam University of Applied Sciences](http://fh-potsdam.de).

Ang core team nga naglangkob nila Prof. Reto Wettach, André Knörig, Jonathan Cohen, ug Stefan Hermann. Ug daghang [banggiitang taw](http://fritzing.org/about/people/) nga naka amot niini sa tibuok tuig.

Ang Fritzing app kay naka sulat sa babaw sa [Qt cross-platform framework](http://qt-project.org).

# Licensing

Ang tinubdan sa code sa Fritzing kay lisinsyado sa ilawm sa GNU GPL v3,ang dokumentasyon ug ang part design sa ilawm sa Creative Commons Attribution-ShareALike 3.0 Unported. Ang tibuok teksto niining mga lesinsyaha kay gipdala kauban sa pad download.

Kini nag pasabot nga ikaw maka himu ug imohang kaugalingong variation sa Fritzing, kung imong kaming e credit ug sa pag mantala niini sa ilawm sa GPL. Kapariho nga, ikaw mag mantala pag balik sa among dokumentasyon, samtang imo kaming e credit, ug imantala sa ilawm sa samang lisensya. Maka mantala ka ug circuits ug diagrams nga imong gimugna sa Fritzing ug sa dihang imong gigamit ang among graphics, sa pag usab samtang ikaw nag hatag namo ug credit  ug mag publish sa imong trabaho sa ilawm sa parihang lisensta. Ang usa ka credit pwede ra sayun sama niini "king larawan gimugna sa Fritzing."

Pangitaa ang  [our FAQs](http://fritzing.org/faq/) para sa uban pang mga detalye.
