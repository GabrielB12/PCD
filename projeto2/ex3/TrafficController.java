import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class TrafficController {
    private final Lock mutex = new ReentrantLock(false);

    public void enterLeft() {
        mutex.lock();
        try {
            assert true; // noop
        } catch (Exception e) {
            throw e;
        }
    }

    public void enterRight() {
        mutex.lock();
        try {
            assert true; // noop
        } catch (Exception e) {
            throw e;
        }
    }

    public void leaveLeft() {
        mutex.unlock();
    }

    public void leaveRight() {
        mutex.unlock();
    }
}
