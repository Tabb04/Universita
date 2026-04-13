import java.util.*;

// ==========================================
// 1. ECCEZIONE PERSONALIZZATA
// ==========================================
class DuplicateUnitException extends Exception {
    public DuplicateUnitException(String message) {
        super(message);
    }
}

// ==========================================
// 2. CLASSE GENERICA PER LA TUPLA
// ==========================================
// T deve estendere Comparable per supportare l'ordinamento richiesto
class SortableTuple<T extends Comparable<T>> implements Comparable<SortableTuple<T>> {
    private final List<T> elements;

    // Costruttore con numero variabile di argomenti (varargs)
    @SafeVarargs
    public SortableTuple(T... elements) {
        this.elements = new ArrayList<>(Arrays.asList(elements));
    }

    // Getters e Setters (T non è un tipo primitivo, es. Integer, String)
    public T get(int index) {
        return elements.get(index);
    }

    public void set(int index, T value) {
        elements.set(index, value);
    }

    // Implementazione dell'ordinamento lessicografico
    @Override
    public int compareTo(SortableTuple<T> other) {
        int minLen = Math.min(this.elements.size(), other.elements.size());
        for (int i = 0; i < minLen; i++) {
            int cmp = this.elements.get(i).compareTo(other.get(i));
            if (cmp != 0) return cmp; // Restituisce il primo elemento che differisce
        }
        // Se tutti gli elementi iniziali sono uguali, vince la tupla più corta
        return Integer.compare(this.elements.size(), other.elements.size());
    }

    // equals e hashCode sono FONDAMENTALI se usiamo la classe come chiave in una Mappa!
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        SortableTuple<?> that = (SortableTuple<?>) o;
        return elements.equals(that.elements);
    }

    @Override
    public int hashCode() {
        return Objects.hash(elements);
    }

    @Override
    public String toString() {
        return elements.toString();
    }
}

// ==========================================
// 3. IL COORDINATORE DEL CENTRO DI CALCOLO
// ==========================================
class ComputeCenter {
    // La nostra combinazione strategica di Collections:
    // Mappa Ordinata (TreeMap) -> Tupla Identificativa -> Coda FIFO (ArrayDeque)
    private Map<SortableTuple<Integer>, Queue<String>> cluster = new TreeMap<>();

    // Registra una nuova unità di calcolo
    public void addUnit(SortableTuple<Integer> unitId) throws DuplicateUnitException {
        if (cluster.containsKey(unitId)) {
            throw new DuplicateUnitException("ATTENZIONE: L'unità " + unitId + " esiste già nel cluster!");
        }
        // Inizializza l'unità con una coda vuota per i futuri task
        cluster.put(unitId, new ArrayDeque<>());
        System.out.println("Nuova unità registrata: " + unitId);
    }

    // Aggiunge un task (Stringa di 16 caratteri) all'unità
    public void addTask(SortableTuple<Integer> unitId, String task) {
        if (!cluster.containsKey(unitId)) {
            System.out.println("Errore: Unità " + unitId + " non trovata.");
            return;
        }
        if (task == null || task.length() != 16) {
            System.out.println("Errore: Il task deve essere una stringa di esattamente 16 caratteri.");
            return;
        }
        
        cluster.get(unitId).offer(task); // .offer() inserisce in fondo alla coda
        System.out.println("Task '" + task + "' aggiunto all'unità " + unitId);
    }

    // Legge il task corrente senza rimuoverlo (peek)
    public void viewCurrentTask(SortableTuple<Integer> unitId) {
        if (!cluster.containsKey(unitId)) return;
        
        Queue<String> queue = cluster.get(unitId);
        if (queue.isEmpty()) {
            System.out.println("Unità " + unitId + ": Nessun task in esecuzione.");
        } else {
            System.out.println("Unità " + unitId + " -> Task corrente: " + queue.peek());
        }
    }

    // Rimuove e completa il task più vecchio (poll)
    public void removeTask(SortableTuple<Integer> unitId) {
        if (!cluster.containsKey(unitId)) return;
        
        Queue<String> queue = cluster.get(unitId);
        String completedTask = queue.poll(); // .poll() estrae e rimuove il primo elemento
        
        if (completedTask != null) {
            System.out.println("Unità " + unitId + " -> Task completato e rimosso: " + completedTask);
        } else {
            System.out.println("Unità " + unitId + " -> Nessun task da rimuovere.");
        }
    }
}

// ==========================================
// 4. MAIN DI TEST
// ==========================================
public class MainSistema {
    public static void main(String[] args) {
        ComputeCenter center = new ComputeCenter();

        // 1. Creiamo gli ID delle unità (Fila, Colonna, Posizione)
        SortableTuple<Integer> unita1 = new SortableTuple<>(1, 2, 5);
        SortableTuple<Integer> unita2 = new SortableTuple<>(3, 4, 12);

        try {
            // Aggiungiamo le unità al cluster
            center.addUnit(unita1);
            center.addUnit(unita2);
            
            // Tentiamo di aggiungere un'unità duplicata per testare l'eccezione
            System.out.println("\n--- Test Eccezione ---");
            center.addUnit(new SortableTuple<>(1, 2, 5)); 
        } catch (DuplicateUnitException e) {
            System.out.println(e.getMessage());
        }

        System.out.println("\n--- Test Assegnazione Task ---");
        // Stringhe di esattamente 16 caratteri
        String taskA = "TASK-0000000000A"; 
        String taskB = "TASK-0000000000B";
        String taskSbagliato = "TASK-CORTO"; // Fallirà la validazione
        
        center.addTask(unita2, taskA);
        center.addTask(unita2, taskB);
        center.addTask(unita2, taskSbagliato);

        System.out.println("\n--- Test Lettura e Rimozione (FIFO) ---");
        center.viewCurrentTask(unita2); // Dovrebbe mostrare il Task A (il primo inserito)
        center.removeTask(unita2);      // Rimuove il Task A
        center.viewCurrentTask(unita2); // Ora il corrente dovrebbe essere il Task B
    }
}