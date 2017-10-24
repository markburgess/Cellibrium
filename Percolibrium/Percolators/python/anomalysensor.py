
import math
import cPickle

FORGETRATE = 0.5

def update_real_Q(qname, newq):
    oldav = 0
    oldvar = 0.1
    [state, oldav, oldvar] = load_special_Q(qname, oldav, oldvar)
    
    if state == True:
        if oldvar == 0:
            oldvar = 0.5
            
        nextav = w_average(newq, oldav)
        newvar = (newq-oldav)*(newq-oldav)
        nextvar = w_average(newvar,oldvar);
        devq = math.sqrt(oldvar)
        
        if devq<0.1:
            devq = 0.1
            
        if newq > (oldav + 3*devq):
            print '!! [pr] Process anomaly '+str(qname)+'_high_anomaly '+'('+str(newq)+' > '+str(oldav)+' + '+str(3*devq)+')'
        
        elif newq < (oldav - 3*devq):
            print '!! [pr] Process anomaly '+str(qname)+'_low_anomaly '+'('+str(newq)+' < '+str(oldav)+' - '+str(3*devq)+')'
        
        save_special_Q(qname,nextav,nextvar)
    
    else:
        nextav = w_average(newq,0);
        nextvar = w_average(newq/2,0);
        
        save_special_Q(qname,nextav,nextvar)


def save_special_Q(name, av, var):
    vec = {'name':name, 'av':av, 'var': var}
    try:
        with open(str(name)+'.pkl', 'wb') as fid:
            cPickle.dump(vec, fid)
            print 'Saved updated values ('+str(av)+','+str(var)+') in tmp/' +str(name)
            return True
    
    except:
        print 'unable to save data'
        return False
    
def load_special_Q(name, oldq, oldvar):
    
    try:
        with open(str(name)+'.pkl', 'rb') as fid:
            data = cPickle.load(fid)
            oldq = data['av']
            oldvar = data['var']
            print 'Got previous average '+str(oldq)+', std_dev '+ str(math.sqrt(oldvar))
            return True, oldq, oldvar
    except:
        print " - no previous value for "+str(name);
        return False, oldq, oldvar
        

def w_average(anew, aold):

    av= 9999999.0
    cf_sane_monitor_limit = 9999999.0
    
    wnew = (1.0 - FORGETRATE);
    wold = FORGETRATE;
    
    av = (wnew * anew + wold * aold);
    
    if av < 0:
        return 0.0;
       
    
    return av

  
if __name__ == "__main__":
    data = [154.4, 155, 144, 234, 0]
    
    for val in data:
        if val !=0:
            update_real_Q('sensor',val)
        



