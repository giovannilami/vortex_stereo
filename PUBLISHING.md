# Pubblicazione manuale su GitHub

Questo repository è stato preparato localmente ma non è collegato a nessun
repository remoto. Nessun file viene pubblicato automaticamente.

## 1. Creazione futura del repository

Quando si decide di pubblicare:

1. Creare su GitHub un repository vuoto chiamato `Vortex_stereo`.
2. Non aggiungere README, licenza o `.gitignore` dalla pagina di creazione,
   perché questi file sono già presenti localmente.
3. Collegare questa cartella al nuovo indirizzo GitHub.
4. Inviare il ramo `main`.

I comandi esatti dipenderanno dal nome dell'account GitHub e saranno eseguiti
solo dopo una conferma esplicita.

## 2. Release v1.0.0

Il repository contiene già il tag locale `v1.0.0`. Per pubblicare la prima
release manualmente:

1. Aprire la pagina **Releases** del repository.
2. Selezionare **Draft a new release**.
3. Selezionare o creare il tag `v1.0.0`.
4. Usare come titolo `Vortex_stereo v1.0.0`.
5. Copiare il testo di `RELEASE_NOTES.md`.
6. Allegare `vortex_stereo.o`, `plugin.json` e, facoltativamente, il pacchetto
   `Vortex_stereo-1.0.0.zip`.
7. Controllare la bozza e premere **Publish release** solo quando desiderato.

La workflow in `.github/workflows/build.yml` esegue soltanto test e
compilazione. Non crea né pubblica GitHub Release.

## 3. NT Gallery

La proposta alla NT Gallery è un passaggio successivo e separato. Va eseguita
solo dopo che repository e release sono pubblici e il binario è stato provato
sull'hardware.
