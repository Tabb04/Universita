// content_script.js

// Aspetta che il DOM sia completamente caricato
window.addEventListener('load', () => {
  // Le tue credenziali
  const username = "mirco.tabarracci";
  const password = "Danielazzo54!";

  // Trova i campi di input per username e password.
  // Questi selettori potrebbero aver bisogno di essere aggiustati se gli ID o i nomi dei campi sono diversi.
  // Dallo screenshot, sembra che i campi siano input generici.
  // Proveremo a selezionarli in base al loro attributo 'name' o 'type'.

  let usernameField = document.querySelector('input[name="username"]');
  let passwordField = document.querySelector('input[name="password"]'); // Spesso i campi password hanno name="password"
  
  // Se non trovati con 'name', proviamo con un approccio più generico basato sulla struttura
  // (questo è meno robusto e dipende dall'ordine dei campi nella pagina)
  if (!usernameField) {
    // Prova a cercare un input di tipo text o email che potrebbe essere il campo username
    const textInputs = document.querySelectorAll('input[type="text"], input[type="email"]');
    if (textInputs.length > 0) {
        // Spesso il primo campo di testo visibile è l'username
        for (let i = 0; i < textInputs.length; i++) {
            if (textInputs[i].offsetParent !== null) { // Controlla se è visibile
                usernameField = textInputs[i];
                break;
            }
        }
    }
  }

  if (!passwordField) {
    passwordField = document.querySelector('input[type="password"]');
  }

  // Inserisci le credenziali se i campi sono stati trovati
  if (usernameField) {
    usernameField.value = username;
    console.log("Username inserito.");
  } else {
    console.error("Campo username non trovato. Controlla i selettori CSS.");
  }

  if (passwordField) {
    passwordField.value = password;
    console.log("Password inserita.");
  } else {
    console.error("Campo password non trovato. Controlla i selettori CSS.");
  }

  // Opzionale: Prova a fare clic sul pulsante di login
  // Dallo screenshot, il pulsante ha il testo "ACCEDI"
  // Potrebbe essere un <input type="submit"> o un <button>
  if (usernameField && passwordField) {
    const loginButton = Array.from(document.querySelectorAll('input[type="submit"], button'))
                             .find(btn => btn.value === 'ACCEDI' || btn.textContent.trim() === 'ACCEDI');

    if (loginButton) {
      // Attendi un breve istante per assicurarti che la pagina abbia processato l'input
      // prima di tentare il submit, specialmente se ci sono script lato client che reagiscono al 'change' o 'input'
      setTimeout(() => {
        loginButton.click();
        console.log("Pulsante ACCEDI cliccato.");
      }, 500); // 500ms di attesa, puoi aggiustarlo se necessario
    } else {
      console.warn("Pulsante ACCEDI non trovato. Dovrai cliccarlo manualmente.");
    }
  }
});
