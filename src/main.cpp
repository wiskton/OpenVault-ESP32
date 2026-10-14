#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp32-hal-bt.h>
#include <esp_system.h>
#include <bootloader_random.h>
#include "openvault/wallet.hpp"
#include "openvault/menu.hpp"
#include <algorithm>
#include <cstring>

SET_LOOP_TASK_STACK_SIZE(16384);
namespace {
using openvault::Wallet;
using openvault::wipe;
using openvault::clearSecret;
Wallet wallet;
Preferences store;
M5Canvas canvas(&M5.Display);
enum class Screen {Welcome,Menu,Network,Count,Words,PinSet,PinConfirm,PinUnlock,Letters,Candidates,Address,QR,WaitPSBT,Review,ConfirmSign,ConfirmExport,ConfirmErase,ConfirmBrightness,Message,Fatal};
Screen screen=Screen::Welcome,afterMessage=Screen::Menu;
constexpr uint16_t bg=0x0842,green=0x47F1,bar=0x126A;
const char* menus[]={"Criar carteira","Importar seed","Desbloquear","Receber BTC","Assinar PSBT","Ver backup","Exportar xpub","Bloquear","Apagar carteira","Trocar PIN"};
constexpr size_t menuCount=sizeof(menus)/sizeof(menus[0]);
size_t selected=0,page=0,wordIndex=0,wordsSeen=0,wordTarget=12,reviewSeen=0;
uint32_t addressIndex=0,lastTouch=0,blockedUntil=0;
int digit=0,letter=0;
bool saved=false,creating=false,importing=false,mainnet=false,heldA=false,heldB=false,canDraw=false;
std::string pin,firstPin,prefix,phrase,message,rx;
std::vector<uint8_t> blob;
std::vector<size_t> candidates;
size_t candidateIndex=0;
std::vector<std::string> reviewLines;
std::vector<openvault::Payment> payments;
uint64_t reviewFee=0;
bool discardRx=false,addressExportPending=false;
int pendingBrightness=100;
const char* alphabet="abcdefghijklmnopqrstuvwxyz";

void render();
void info(const std::string& text,Screen next=Screen::Menu){message=text;afterMessage=next;screen=Screen::Message;page=0;render();}
std::vector<std::string> chunks(const std::string& text,size_t width=18){
 std::vector<std::string> out;
 for(size_t start=0;start<text.size();){
  size_t end=std::min(start+width,text.size());
  if(end<text.size()){size_t space=text.rfind(' ',end);if(space!=std::string::npos&&space>start)end=space;}
  out.push_back(text.substr(start,end-start));start=end;while(start<text.size()&&text[start]==' ')++start;
 }
 return out;
}
void addLines(std::vector<std::string>& lines,const std::string& text){auto more=chunks(text);lines.insert(lines.end(),more.begin(),more.end());}
void label(int y,const std::string& text,uint16_t color=TFT_WHITE){canvas.setTextColor(color,bg);canvas.setCursor(8,y);canvas.print(text.c_str());}
void head(const std::string& title,const std::string& right=""){
 canvas.fillScreen(bg);canvas.setTextSize(2);canvas.setTextWrap(false);canvas.fillRect(0,0,240,25,bar);
 canvas.setTextColor(TFT_WHITE,bar);canvas.setCursor(6,5);canvas.print(title.c_str());
 canvas.setTextColor(TFT_YELLOW,bar);canvas.setCursor(176,5);canvas.print(right.c_str());
}
void foot(const std::string& text){canvas.drawFastHLine(0,110,240,0x3186);label(116,text,0xBDF7);}
void pageText(const std::string& title,const std::vector<std::string>& lines,const std::string& footer){
 size_t count=std::max(size_t(1),(lines.size()+2)/3);if(page>=count)page=0;
 head(title,count>1?std::to_string(page+1)+"/"+std::to_string(count):"");
 for(size_t i=0;i<3;++i)if(page*3+i<lines.size())label(32+i*26,lines[page*3+i]);foot(footer);
}
void render(){
 if(!canDraw)return;
 switch(screen){
 case Screen::Welcome:pageText("OPENVAULT",chunks("Carteira Bitcoin 0.3.0. Software experimental, sem auditoria. Anote sua seed. PIN nao bloqueia extracao fisica."),"A:entra B:pagina");break;
 case Screen::Menu:{
  const auto items=openvault::walletMenu(saved,wallet.ready());if(selected>=items.size())selected=0;
  head("OPENVAULT",std::to_string(selected+1)+"/"+std::to_string(items.size()));
  size_t first=(selected/3)*3;
  for(size_t row=0;row<3;++row){size_t i=first+row;if(i>=items.size())break;int y=32+row*26;bool active=i==selected;
   if(active)canvas.fillRoundRect(4,y-3,232,24,3,green);canvas.setTextColor(active?bg:TFT_WHITE,active?green:bg);canvas.setCursor(8,y);canvas.print(active?">":" ");canvas.print(menus[items[i]]);}
  foot(wallet.ready()?"A:abre B:proximo":"BLOQ. A:abre B:+");break;}
 case Screen::Network:head("REDE");label(34,mainnet?"> Bitcoin real":"> Testnet",green);label(62,"B: trocar rede");label(87,"A: confirmar");foot("Segure B:cancelar");break;
 case Screen::Count:head(importing?"IMPORTAR":"CRIAR");label(35,std::to_string(wordTarget)+" palavras",green);label(63,"B: 12 ou 24");label(88,"A: continuar");foot("Segure B:cancelar");break;
 case Screen::Words:{head("ANOTE A SEED",std::to_string(wordIndex+1)+"/"+std::to_string(wallet.wordCount()));auto word=wallet.mnemonicWord(wordIndex);canvas.setTextSize(3);label(49,word,green);clearSecret(word);canvas.setTextSize(2);foot(wordIndex+1==wallet.wordCount()?"Segure A:continua":"B:proxima palavra");break;}
 case Screen::PinSet:case Screen::PinConfirm:case Screen::PinUnlock:{
  head(screen==Screen::PinSet?"NOVO PIN":screen==Screen::PinConfirm?"REPITA PIN":"SEU PIN");label(35,"PIN de 8 digitos");
  std::string dots(pin.size(),'*');dots+=char('0'+digit);label(61,dots,green);clearSecret(dots);label(87,"A:fixa B:troca");foot("Segure B:cancela");break;}
 case Screen::Letters:head("PALAVRA",std::to_string(wordIndex+1)+"/"+std::to_string(wordTarget));label(34,prefix+"_",green);label(61,std::string("Letra: ")+alphabet[letter]);label(87,"A:fixa B:troca");foot("Segure B:apaga");break;
 case Screen::Candidates:head("ESCOLHA",std::to_string(wordIndex+1)+"/"+std::to_string(wordTarget));label(35,bip39_get_word_by_index(nullptr,candidates[candidateIndex]),green);label(63,std::to_string(candidateIndex+1)+" de "+std::to_string(candidates.size()));foot("A:aceita B:troca");break;
 case Screen::Address:{auto lines=chunks(wallet.address(addressIndex));lines.insert(lines.begin(),"Endereco #"+std::to_string(addressIndex));pageText(wallet.mainnet()?"BITCOIN":"TESTNET",lines,addressExportPending?"Segure A:envia USB":"A:QR B:pagina/+");break;}
 case Screen::QR:{canvas.fillScreen(TFT_WHITE);std::string address=wallet.address(addressIndex);canvas.qrcode(address.c_str(),52,0,135,4);break;}
 case Screen::WaitPSBT:pageText("ASSINAR",chunks("Envie a PSBT pelo companion USB. Voce revisara cada saida, destino e taxa nesta tela."),"A:voltar B:pagina");break;
 case Screen::Review:pageText("REVISAR",reviewLines,page+1==(reviewLines.size()+2)/3?"A:confirmar B:+":"B:proxima pagina");break;
 case Screen::ConfirmSign:pageText("CONFIRMAR",{"Todos os destinos","e a taxa conferem?","Segure A: ASSINAR"},"B:cancela");break;
 case Screen::ConfirmExport:pageText("EXPORTAR",{"Exportar xpub USB?","Revela historico","da sua carteira."},"Segure A:autoriza");break;
 case Screen::ConfirmErase:pageText("APAGAR COFRE",{"Seed/PIN serao","removidos. Tem","backup das palavras?"},"Segure A:apaga");break;
 case Screen::ConfirmBrightness:pageText("BRILHO",{"Novo brilho:",std::to_string(pendingBrightness)+" de 255","Confirme na placa."},"Segure A:confirma");break;
 case Screen::Message:pageText("OPENVAULT",chunks(message),"A:voltar B:pagina");break;
 case Screen::Fatal:pageText("ERRO",chunks(message),"Reinicie aparelho");break;
 }
 canvas.pushSprite(0,0);
}
void busy(const char* text){head("AGUARDE");label(40,text);foot("Nao desconecte");canvas.pushSprite(0,0);}
void randomBytes(uint8_t* out,size_t len){
 // No RF/ADC/I2S task is active: mic and speaker are disabled and no battery ADC is sampled.
 bootloader_random_enable();delay(20);esp_fill_random(out,len);bootloader_random_disable();
}
void resetEntry(){clearSecret(pin);clearSecret(firstPin);clearSecret(prefix);clearSecret(phrase);digit=0;letter=0;}
void cancel(){
 if(addressExportPending||screen==Screen::ConfirmExport||screen==Screen::ConfirmBrightness)Serial.println("CANCELED");
 addressExportPending=false;
 if(screen==Screen::Review||screen==Screen::ConfirmSign||screen==Screen::WaitPSBT){wallet.cancelReview();Serial.println("CANCELED");}
 if(creating){wallet.lock();creating=false;}
 resetEntry();screen=Screen::Menu;page=0;render();
}
void startPin(Screen target){clearSecret(pin);digit=0;screen=target;render();}
void persist(){
 busy("Salvando cofre...");std::array<uint8_t,32> random{};randomBytes(random.data(),random.size());std::vector<uint8_t> encrypted;
 bool ok=wallet.seal(firstPin,random,encrypted);wipe(random.data(),random.size());
 if(ok){ok=store.putBytes("vault",encrypted.data(),encrypted.size())==encrypted.size();if(ok){std::vector<uint8_t> check(encrypted.size());ok=store.getBytes("vault",check.data(),check.size())==check.size()&&check==encrypted;}}
 resetEntry();if(!ok){wallet.lock();creating=false;info("Falha ao salvar. Cofre nao confirmado.");return;}
 blob=encrypted;if(!saved){store.putUInt("receive",0);addressIndex=0;}saved=true;creating=false;store.putUInt("attempts",0);lastTouch=millis();info("Carteira salva. Confira seu backup de palavras. Ela sera bloqueada apos 90s sem uso.");
}
void finishPin(){
 if(screen==Screen::PinSet){firstPin=pin;startPin(Screen::PinConfirm);return;}
 if(screen==Screen::PinConfirm){
  if(pin!=firstPin){clearSecret(firstPin);info("PINs diferentes. Digite um novo PIN.",Screen::PinSet);clearSecret(pin);digit=0;return;}
  persist();return;
 }
 busy("Desbloqueando...");bool ok=wallet.unlock(blob,pin);clearSecret(pin);digit=0;
 if(ok){store.putUInt("attempts",0);mainnet=wallet.mainnet();addressIndex=store.getUInt("receive",0);lastTouch=millis();info("Carteira desbloqueada.");}
 else {uint32_t failures=std::min(uint32_t(100),store.getUInt("attempts",0)+1);store.putUInt("attempts",failures);if(failures>=5)blockedUntil=millis()+30000;info("PIN incorreto ou cofre adulterado. Tente novamente.");}
}
void acceptWord(){
 if(!phrase.empty())phrase+=' ';phrase+=bip39_get_word_by_index(nullptr,candidates[candidateIndex]);clearSecret(prefix);letter=0;
 ++wordIndex;
 if(wordIndex<wordTarget){screen=Screen::Letters;return;}
 busy("Validando seed...");bool ok=wallet.importWords(phrase,mainnet);clearSecret(phrase);
 if(!ok){creating=false;wallet.lock();info("Seed invalida: palavras ou checksum. Importe novamente.");return;}
 wordIndex=wordsSeen=0;screen=Screen::Words;
}
void shortA(){
 lastTouch=millis();
 switch(screen){
 case Screen::Welcome:screen=Screen::Menu;page=0;break;
 case Screen::Menu:{
  const auto items=openvault::walletMenu(saved,wallet.ready());if(selected>=items.size())selected=0;const auto action=items[selected];
  if(action==0||action==1){if(saved){info("Ja existe cofre. Use Desbloquear. Para substituir, faca backup e Apagar carteira.");return;}creating=true;importing=action==1;mainnet=false;screen=Screen::Network;}
  else if(action==2){if(!saved){info("Nenhum cofre salvo. Use Criar carteira ou Importar seed.");return;}if(blockedUntil&&int32_t(millis()-blockedUntil)<0){info("Muitas tentativas. Aguarde 30 segundos.");return;}startPin(Screen::PinUnlock);return;}
  else if(action==3){if(!wallet.ready()){info("Desbloqueie a carteira primeiro.");return;}page=0;screen=Screen::Address;}
  else if(action==4){if(!wallet.ready()){info("Desbloqueie a carteira primeiro.");return;}screen=Screen::WaitPSBT;page=0;}
  else if(action==5){if(!wallet.ready()){info("Desbloqueie a carteira primeiro.");return;}wordIndex=wordsSeen=0;screen=Screen::Words;}
  else if(action==6){if(!wallet.ready()){info("Desbloqueie a carteira primeiro.");return;}screen=Screen::ConfirmExport;}
  else if(action==7){wallet.lock();resetEntry();info("Carteira bloqueada.");return;}
  else if(action==9){if(!wallet.ready()){info("Desbloqueie a carteira primeiro.");return;}startPin(Screen::PinSet);return;}
  else if(action==8){if(!wallet.ready()){info("Desbloqueie antes de apagar.");return;}screen=Screen::ConfirmErase;}
  break;}
 case Screen::Network:wordTarget=12;screen=Screen::Count;break;
 case Screen::Count:
  if(importing){wordIndex=0;clearSecret(phrase);clearSecret(prefix);letter=0;screen=Screen::Letters;}
  else {busy("Criando carteira...");uint8_t entropy[32];randomBytes(entropy,wordTarget==24?32:16);bool ok=wallet.setEntropy(entropy,wordTarget==24?32:16,mainnet);wipe(entropy,sizeof(entropy));if(!ok){creating=false;info("Falha ao criar carteira.");return;}wordIndex=wordsSeen=0;screen=Screen::Words;}
  break;
 case Screen::PinSet:case Screen::PinConfirm:case Screen::PinUnlock:pin+=char('0'+digit);digit=0;if(pin.size()==8){finishPin();return;}break;
 case Screen::Letters:{prefix+=alphabet[letter];letter=0;if(prefix.size()>=3){candidates.clear();for(size_t i=0;i<2048;++i){const char* word=bip39_get_word_by_index(nullptr,i);if(word&&std::strncmp(word,prefix.c_str(),prefix.size())==0)candidates.push_back(i);}if(candidates.empty()){clearSecret(prefix);info("Nenhuma palavra. Digite novamente.",Screen::Letters);return;}candidateIndex=0;screen=Screen::Candidates;}break;}
 case Screen::Candidates:acceptWord();break;
 case Screen::Address:screen=Screen::QR;break;
 case Screen::QR:screen=Screen::Address;break;
 case Screen::Review:if(page+1==(reviewLines.size()+2)/3&&reviewSeen>=page){screen=Screen::ConfirmSign;page=0;}break;
 case Screen::WaitPSBT:screen=Screen::Menu;page=0;break;
 case Screen::Message:screen=afterMessage;page=0;break;
 default:break;
 }
 render();
}
void shortB(){
 lastTouch=millis();
 switch(screen){
 case Screen::Menu:selected=(selected+1)%openvault::walletMenu(saved,wallet.ready()).size();break;
 case Screen::Network:mainnet=!mainnet;break;
 case Screen::Count:wordTarget=wordTarget==12?24:12;break;
 case Screen::Words:wordIndex=(wordIndex+1)%wallet.wordCount();wordsSeen=std::max(wordsSeen,wordIndex);break;
 case Screen::PinSet:case Screen::PinConfirm:case Screen::PinUnlock:digit=(digit+1)%10;break;
 case Screen::Letters:letter=(letter+1)%26;break;
 case Screen::Candidates:candidateIndex=(candidateIndex+1)%candidates.size();break;
 case Screen::Address:if(++page>=((chunks(wallet.address(addressIndex)).size()+1+2)/3)){page=0;if(!addressExportPending){addressIndex=(addressIndex+1)%100000;store.putUInt("receive",addressIndex);}}break;
 case Screen::QR:screen=Screen::Address;page=0;break;
 case Screen::Review:page=(page+1)%std::max(size_t(1),(reviewLines.size()+2)/3);reviewSeen=std::max(reviewSeen,page);break;
 case Screen::ConfirmSign:case Screen::ConfirmExport:case Screen::ConfirmErase:case Screen::ConfirmBrightness:cancel();return;
 case Screen::Welcome:case Screen::Message:case Screen::WaitPSBT:++page;break;
 default:break;
 }
 render();
}
void longA(){
 lastTouch=millis();
 if(screen==Screen::Address&&addressExportPending){
  const size_t pages=(chunks(wallet.address(addressIndex)).size()+1+2)/3;
  if(page+1!=pages)return;
  Serial.print("ADDRESS ");Serial.print(wallet.mainnet()?"BTC ":"TEST ");Serial.print(addressIndex);Serial.print(" ");Serial.println(wallet.address(addressIndex).c_str());
  addressExportPending=false;screen=Screen::QR;render();return;
 }
 if(screen==Screen::ConfirmBrightness){M5.Display.setBrightness(pendingBrightness);store.putUInt("brightness",pendingBrightness);Serial.println("CONFIG_OK brightness");info("Brilho atualizado.");return;}

 if(screen==Screen::Words){
  if(wordIndex+1!=wallet.wordCount()||wordsSeen+1<wallet.wordCount())return;
  if(creating){clearSecret(firstPin);startPin(Screen::PinSet);}else {screen=Screen::Menu;render();}return;
 }
 if(screen==Screen::ConfirmExport){Serial.print("XPUB ");Serial.print(wallet.fingerprint().c_str());Serial.print(wallet.mainnet()?" 84h/0h/0h ":" 84h/1h/0h ");Serial.println(wallet.accountXpub().c_str());info("Xpub exportado pela USB. Nenhuma chave privada foi enviada.");return;}
 if(screen==Screen::ConfirmSign){busy("Assinando...");std::string signedTx;bool ok=wallet.signReviewed(signedTx);if(ok){Serial.print("SIGNED ");Serial.println(signedTx.c_str());info("PSBT assinada e enviada pela USB. A transmissao e feita no computador.");}else{Serial.println("ERROR assinatura");info("Nao foi possivel assinar.");}return;}
 if(screen==Screen::ConfirmErase){bool ok=store.remove("vault");if(ok){wallet.lock();resetEntry();blob.clear();saved=false;creating=false;store.remove("attempts");store.remove("receive");info("Cofre removido. A seed anotada permite restaurar.");}else info("Falha ao remover cofre.");return;}
}
void longB(){
 lastTouch=millis();
 if(screen==Screen::Letters){if(!prefix.empty())prefix.pop_back();else cancel();letter=0;render();return;}
 if(screen==Screen::Candidates){clearSecret(prefix);letter=0;screen=Screen::Letters;render();return;}
 cancel();
}
void command(const std::string& cmd){
 if(cmd=="STATUS"){
  Serial.print("STATUS ");Serial.print(saved?"saved ":"empty ");Serial.print(wallet.ready()?"unlocked":"locked");
  if(wallet.ready()){Serial.print(wallet.mainnet()?" BTC ":" TEST ");Serial.print(wallet.fingerprint().c_str());}
  Serial.print(" brightness:");Serial.println(store.getUInt("brightness",100));return;
 }
 const bool safeScreen=screen==Screen::Menu||screen==Screen::Welcome||screen==Screen::Address||screen==Screen::QR||(screen==Screen::Message&&afterMessage==Screen::Menu&&!creating);
 if(cmd=="ADDRESS"){
  if(!wallet.ready()||!saved||!safeScreen){Serial.println("ERROR desbloqueie_e_volte_ao_menu");return;}
  addressExportPending=true;screen=Screen::Address;page=0;Serial.println("PENDING address");render();return;
 }
 if(cmd=="REQUEST_XPUB"){
  if(!wallet.ready()||!saved||!safeScreen){Serial.println("ERROR desbloqueie_e_volte_ao_menu");return;}
  screen=Screen::ConfirmExport;page=0;Serial.println("PENDING xpub");render();return;
 }
 if(cmd.compare(0,11,"BRIGHTNESS ")==0){
  const auto text=cmd.substr(11);
  if(text.empty()||text.size()>3||!std::all_of(text.begin(),text.end(),[](char c){return c>='0'&&c<='9';})||!safeScreen){Serial.println("ERROR configuracao_indisponivel");return;}
  int value=std::atoi(text.c_str());if(value<40||value>200){Serial.println("ERROR brilho_40_200");return;}
  pendingBrightness=value;screen=Screen::ConfirmBrightness;page=0;Serial.println("PENDING brightness");render();return;
 }
 if(cmd=="PING"){Serial.println("OPENVAULT 0.3.0");return;}
 if(cmd.compare(0,5,"SIGN ")==0){
  if(screen!=Screen::WaitPSBT||!wallet.ready()){Serial.println("ERROR selecione_Assinar_PSBT_na_placa");return;}
  std::string error;busy("Validando PSBT...");
  if(!wallet.review(cmd.substr(5),payments,reviewFee,error)){Serial.print("ERROR ");Serial.println(error.c_str());info(error);return;}
  reviewLines.clear();addLines(reviewLines,wallet.mainnet()?"REDE: BITCOIN REAL":"REDE: TESTNET");
  for(size_t i=0;i<payments.size();++i){const auto& out=payments[i];addLines(reviewLines,std::string(out.change?"TROCO VALIDADO ":"DESTINO ")+std::to_string(i+1));addLines(reviewLines,out.address);addLines(reviewLines,"Valor em satoshis:");addLines(reviewLines,std::to_string(out.sats));}
  addLines(reviewLines,"Taxa em satoshis:");addLines(reviewLines,std::to_string(reviewFee));addLines(reviewLines,"Confira todos os caracteres dos enderecos.");
  addLines(reviewLines,"Versao TX: "+std::to_string(wallet.reviewedVersion()));addLines(reviewLines,"Locktime: "+std::to_string(wallet.reviewedLocktime()));
  page=reviewSeen=0;screen=Screen::Review;Serial.println("REVIEW_READY");render();return;
 }
 Serial.println("ERROR comando_invalido");
}
void serialInput(){
 size_t budget=512;while(Serial.available()&&budget--){char c=char(Serial.read());if(c=='\r')continue;if(c=='\n'){
  if(discardRx)Serial.println("ERROR mensagem_muito_grande");else command(rx);rx.clear();discardRx=false;
 }else if(!discardRx){if(rx.size()>=24005){rx.clear();discardRx=true;}else rx+=c;}}
}
}
void setup(){
 pinMode(4,OUTPUT);digitalWrite(4,HIGH);
 auto cfg=M5.config();cfg.fallback_board=m5::board_t::board_M5StickCPlus2;cfg.serial_baudrate=115200;cfg.internal_mic=false;cfg.internal_spk=false;M5.begin(cfg);
 WiFi.mode(WIFI_OFF);btStop();M5.Display.setRotation(1);M5.Display.setBrightness(100);canvas.setColorDepth(16);canDraw=canvas.createSprite(240,135)!=nullptr;
 if(!canDraw){M5.Display.print("Erro de memoria");return;}
 busy("Autoteste...");wally_init(0);uint8_t random[32];randomBytes(random,sizeof(random));bool ok=wally_secp_randomize(random,sizeof(random))==WALLY_OK;wipe(random,sizeof(random));ok=ok&&Wallet::selfTest();
 if(!ok){message="Autoteste criptografico falhou. Operacoes bloqueadas.";screen=Screen::Fatal;render();Serial.println("SELFTEST_FAILED");return;}
 if(!store.begin("openvault",false)){message="Erro ao abrir armazenamento.";screen=Screen::Fatal;render();return;}
 M5.Display.setBrightness(std::min(uint32_t(200),std::max(uint32_t(40),store.getUInt("brightness",100))));
 size_t size=store.getBytesLength("vault");if(size){saved=true;if(size==120){blob.resize(size);store.getBytes("vault",blob.data(),size);}else{message="Cofre corrompido. Nao foi apagado. Restaure pelo backup da seed.";screen=Screen::Fatal;render();return;}}
 if(store.getUInt("attempts",0)>=5)blockedUntil=millis()+30000;
 rx.reserve(1024);lastTouch=millis();render();Serial.println("OPENVAULT 0.3.0 SELFTEST_OK READY");
}
void loop(){
 M5.update();if(!canDraw||screen==Screen::Fatal){delay(20);return;}
 if(M5.BtnA.pressedFor(800)&&!heldA){heldA=true;longA();}
 if(M5.BtnB.pressedFor(800)&&!heldB){heldB=true;longB();}
 if(M5.BtnA.wasReleased()){if(!heldA)shortA();heldA=false;}
 if(M5.BtnB.wasReleased()){if(!heldB)shortB();heldB=false;}
 if(wallet.ready()&&millis()-lastTouch>=90000){wallet.lock();resetEntry();creating=false;addressExportPending=false;Serial.println("LOCKED timeout");info("Bloqueio automatico. Digite o PIN para voltar.");}
 serialInput();delay(10);
}
